#include "tail_wrap.h"
#include "tail_file_scan.h"

#define BUFFER_LENGTH 1024
#define MAX_LINE_SCAN 5

sig_atomic_t tail_stop_flag = 0;
uint32_t debug_level = 4;
int buff_len = BUFFER_LENGTH - 1;

static void
set_debug_level (uint32_t level)
{
	debug_level = level;
}

void sig_handle (int signum)
{
	tail_stop_flag = 1;
}

int main (int argc, char** argv)
{
	//Take argument
	//Open a file
	//Check whether regular file
	//scan 'n' lines and display
	//Watch for next file event in while loop

	char* file_name = argv[1];
	sig_vector_t sig_vec = {sig_handle};
	tail_ctx_t *ctx = NULL;
	char buf[BUFFER_LENGTH] = {0};
	int len = buff_len;

	set_debug_level(TAIL_LOG_ERROR);

	if (!file_name) {
		tail_log(TAIL_LOG_ERROR, "No file provided for tailing\n");
		return EXIT_FAILURE;
	}

	ctx = tail_ctx_init();
	if (!ctx) {
		tail_log(TAIL_LOG_ERROR, "System out of memory!");
		return EXIT_FAILURE;
	}

	if (!tail_setup_signals(&sig_vec)) {
		tail_log(TAIL_LOG_WARN, "Could not install properly signal handlers\n");
	}

	if (tail_file_open(ctx, file_name) != TAIL_OK) {
		tail_log(TAIL_LOG_ERROR, "Could not open the file for tailing\n");
		goto l_fopen;
	}

	if (tail_file_event_init(ctx) != TAIL_OK) {
		tail_log(TAIL_LOG_ERROR, "Could not initialize event listener for file\n");
		goto l_fevent;
	}

	if (tail_create_watch(ctx, file_name, TAIL_EVENT_MASK)) {
		tail_log(TAIL_LOG_ERROR, "Unable to create an event watch\n");
		goto l_fwatch;
	}

	data_buffer_t data_buf;
	tail_scan_n_lines(ctx, MAX_LINE_SCAN, &data_buf);
	printf("%s", data_buf.buffer);

	while ((tail_event_watch(ctx) == TAIL_OK) && !tail_stop_flag) {
		len = buff_len;
		tail_read_diff(ctx, buf, &len);
		while(len) {
			buf[len] = '\0';
			printf("%s", buf);
			tail_read_diff(ctx, buf, &len);
		}
	}

l_fwatch:
	tail_file_event_deinit(ctx);
l_fevent:
	tail_file_close(ctx);
l_fopen:
	tail_ctx_deinit(ctx);

	return EXIT_SUCCESS;
}
