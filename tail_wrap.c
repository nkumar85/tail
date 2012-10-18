#include "tail_wrap.h"

//May be logging also
#define TAIL_RETURN_NULL_ARG(value,ret)	\
	if (!value) {						\
		tail_log(TAIL_LOG_ERROR, "%s - Invalid Tail Context provided\n", TAIL_CUR_FUNC);	\
		return ret;						\
	}									\

tail_return_e
tail_map_sys_err (const char* func, const char* syscall_text)
{
	//Log and return error
	int err_no = errno;
	tail_log (TAIL_LOG_SYSCALL, "%s - %s -", err_no, func, syscall_text);	
	errno = 0; /* Make sure clear errno */
	return TAIL_SYS_ERR;
}

static BOOL
__setup_signals (const struct sigaction* sig_act)
{
	if (sigaction(SIGTERM, sig_act, NULL))		
		return B_FALSE;
		
	if (sigaction(SIGINT, sig_act, NULL))
		return B_FALSE;

	return B_TRUE;
}

static void
__init_file_handle (tail_file_handle_t* f_handle)
{
	f_handle->fd = -1;
	memset(&(f_handle->f_stat), 0, sizeof(f_handle->f_stat));
}

static inline BOOL
__file_handle_valid (tail_file_handle_t* f_handle)
{
	return (f_handle->fd >= 0);
}

static void
__deinit_file_handle (tail_file_handle_t* f_handle)
{
	if (__file_handle_valid(f_handle)) {
		close(f_handle->fd);
	}
	f_handle->fd = -1;
}

static inline BOOL
__check_flag (mode_t mask, mode_t flag)
{
	return (mask & flag);
}

static inline BOOL
__file_stat (tail_file_handle_t* f_handle)
{	
	if (fstat(f_handle->fd, &(f_handle->f_stat))) {
		return B_FALSE;
	}

	return B_TRUE;
}

static inline ssize_t
__file_handle_read (tail_file_handle_t* f_handle, void* buf, size_t count, off_t offset)
{
	/*
	 * pread cannot take negative offset.
	 * This can be used by API to distinguish 
	 * whether user needs pread or read
	 */
	if (offset < 0) {
		return read(f_handle->fd, buf, count);
	}

	return pread(f_handle->fd, buf, count, offset);
}

tail_ctx_t*
tail_ctx_init (void)
{
	tail_ctx_t* tail_context =
				TAIL_MEM_ZALLOC(tail_ctx_t, 1, sizeof(tail_ctx_t));
	if (tail_context) {
		__init_file_handle(&(tail_context->f_handle));
		return tail_context;
	}

	tail_log(TAIL_LOG_ERROR,"%s -- System Out of Memory : \n", TAIL_CUR_FUNC);
	return NULL;
}

BOOL
tail_setup_signals(sig_vector_t* sig_vec)
{
	if (sig_vec) {
		struct sigaction sig_act;
		memset(&sig_act, 0, sizeof(struct sigaction));
		
		if (!(sig_vec->sig_f_handle)) {
			return B_FALSE;
		}

		/* Currently common handler for all signals */
		sig_act.sa_handler = sig_vec->sig_f_handle;
		if (__setup_signals(&sig_act)) {			
			return B_TRUE;
		}

		tail_map_sys_err(TAIL_CUR_FUNC, "sigaction");
	}

	return B_FALSE;
}

tail_return_e
tail_file_open (tail_ctx_t* t_ctx, const char* f_name)
{
	TAIL_RETURN_NULL_ARG(t_ctx, TAIL_INVALID_CTX);

	/*
	 * Invalid or NULL filename will be handled by syscall.
	 * The probability of having NULL filename is very rare.
	 * Hence it is OK to offload error check to syscall.
	 */
	int fd = open(f_name, O_RDONLY);
	if (fd == -1) {
		return tail_map_sys_err(TAIL_CUR_FUNC, "open");
	}
	t_ctx->f_handle.fd = fd;

	/*
	 * Since __deinit_file_handle() calls close() system call,
	 * we need to save the status of stat before calling
	 * __deinit_file_handle to make sure errno does not get
	 * mixed up!
	 */
	if (!__file_stat(&(t_ctx->f_handle))) {
		tail_return_e temp = tail_map_sys_err(TAIL_CUR_FUNC, "fstat");
		__deinit_file_handle(&(t_ctx->f_handle));	
		return temp;
	}
	
	/* Need to check if owner has read permission on file */
	if (!__check_flag(t_ctx->f_handle.f_stat.st_mode, S_IRUSR)) {
		tail_log(TAIL_LOG_ERROR, "The given file is not regular file\n");
		__deinit_file_handle(&(t_ctx->f_handle));
		return TAIL_NOT_REG_FILE;
	}

	t_ctx->priv_data.cur_off = tail_file_length(t_ctx);

	return TAIL_OK;
}

BOOL
tail_is_regular_file (tail_ctx_t* t_ctx)
{
	TAIL_RETURN_NULL_ARG(t_ctx, B_FALSE);

	if (__file_handle_valid (&(t_ctx->f_handle))) {
		return __check_flag(t_ctx->f_handle.f_stat.st_mode, S_IFREG);
	}

	return B_FALSE;
}

BOOL
tail_refresh_file (tail_ctx_t* t_ctx)
{
	tail_file_handle_t* f_handle = NULL;

	TAIL_RETURN_NULL_ARG(t_ctx, B_FALSE);

	f_handle = TAIL_FILE_HANDLE(t_ctx);
	if (__file_handle_valid(f_handle)) {
		if (__file_stat(f_handle)) {
			return B_TRUE;
		}
		tail_map_sys_err(TAIL_CUR_FUNC, "fstat");
	}

	return B_FALSE;
}

off_t
tail_file_length (tail_ctx_t* t_ctx)
{
	tail_file_handle_t* f_handle = NULL;

	TAIL_RETURN_NULL_ARG(t_ctx, (off_t) -1);

	f_handle = TAIL_FILE_HANDLE(t_ctx);
	if (!__file_handle_valid(f_handle)) {
		return (off_t)-1;
	}

	return t_ctx->f_handle.f_stat.st_size;
}

off_t
tail_seek_file_offset (tail_ctx_t* t_ctx, off_t offset, int pos)
{
	off_t off = (off_t)-1;

	TAIL_RETURN_NULL_ARG(t_ctx, (off_t)-1);
	if (__file_handle_valid(TAIL_FILE_HANDLE(t_ctx))) {		
		off = lseek(t_ctx->f_handle.fd, offset, pos);
		if(off == (off_t)-1) {
			tail_map_sys_err(TAIL_CUR_FUNC, "lseek");
		} else {
			return off;
		}
	}

	tail_log(TAIL_LOG_ERROR, "%s -- Invalid file handler\n", TAIL_CUR_FUNC);
	return off;
}

ssize_t
tail_file_read (tail_ctx_t* t_ctx, void* buf, size_t n_bytes, off_t offset)
{
	TAIL_RETURN_NULL_ARG(t_ctx, (ssize_t)-1);
	tail_file_handle_t *f_handle = TAIL_FILE_HANDLE(t_ctx);
	ssize_t ret = -1;

	if (!__file_handle_valid(f_handle)) {
		tail_log(TAIL_LOG_ERROR, "%s -- Invalid file handler\n", TAIL_CUR_FUNC);
		goto error;
	}

	ret = __file_handle_read (f_handle, buf, n_bytes, offset);
	if (ret == -1) {
		//log
	}

error:
	return ret;
}

void
tail_file_close (tail_ctx_t* t_ctx)
{
	if (t_ctx) {
		__deinit_file_handle(&(t_ctx->f_handle));
	}
}

tail_return_e
tail_file_event_init(tail_ctx_t* t_ctx)
{
	if (t_ctx) {
		t_ctx->f_evt_handle.evt_fd = -1;
		t_ctx->f_evt_handle.watch_fd = -1;
		memset(&(t_ctx->f_evt_handle.i_evt), 0,
				sizeof(struct inotify_event));
		return TAIL_OK;
	}

	return TAIL_INVALID_CTX;
}

tail_return_e
tail_create_watch(tail_ctx_t* t_ctx, const char* f_name, uint32_t mask)
{
	if (t_ctx) {
		if ((t_ctx->f_evt_handle.evt_fd = inotify_init()) == -1) {
			return tail_map_sys_err(TAIL_CUR_FUNC, "inotify_init");
		}

		t_ctx->f_evt_handle.watch_fd =
			inotify_add_watch(t_ctx->f_evt_handle.evt_fd, f_name,
								mask);
		if (t_ctx->f_evt_handle.watch_fd == -1) {
			return tail_map_sys_err(TAIL_CUR_FUNC, "inotify_add_watch");
		}

		return TAIL_OK;
	}

	return TAIL_INVALID_CTX;
}

tail_return_e
tail_event_watch(tail_ctx_t* t_ctx)
{
	struct inotify_event *i_event = NULL;
	if (t_ctx) {
		i_event = &(t_ctx->f_evt_handle.i_evt);
		memset(i_event, 0, sizeof(*i_event));
		i_event->wd = t_ctx->f_evt_handle.watch_fd;
		if (read(t_ctx->f_evt_handle.evt_fd, i_event, sizeof(*i_event)) > 0) {
			return TAIL_OK;
		} else {		
			return tail_map_sys_err("tail_event_watch", "read");
		}
	}

	return TAIL_INVALID_CTX;
}

void
tail_file_event_deinit(tail_ctx_t* t_ctx)
{
	if (t_ctx) {
		//validate watch fd
		inotify_rm_watch(t_ctx->f_evt_handle.evt_fd, t_ctx->f_evt_handle.watch_fd);
	}
}

tail_evt_mask
tail_get_event_mask (tail_ctx_t* t_ctx)
{
	if (t_ctx) {
		return (tail_evt_mask)(t_ctx->f_evt_handle.i_evt.mask);
	}

	return (tail_evt_mask)0;
}

tail_return_e
tail_read_diff (tail_ctx_t* t_ctx, void* buf, ssize_t* length)
{
	off_t cur_off = t_ctx->priv_data.cur_off;
	off_t new_off = 0;

	if (t_ctx) {
		tail_refresh_file(t_ctx);
		new_off = tail_file_length(t_ctx);
		if (new_off > cur_off) {
			*length = __file_handle_read(&t_ctx->f_handle, buf, *length, cur_off);
			new_off = cur_off + *length;
		} else {
			*length = 0;
		}
		t_ctx->priv_data.cur_off = new_off;
		return TAIL_OK;
	}
	return TAIL_INVALID_CTX;
}

void
tail_ctx_deinit (tail_ctx_t* t_ctx)
{
	if (t_ctx) {
		TAIL_MEM_FREE(t_ctx)
	}
}
