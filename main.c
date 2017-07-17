#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>
#include <sys/resource.h>

/*
void log_open(const char* name);
void log_close(void);
void log_write(int priority, const char* message);

void log_open(const char* name)
{
	openlog(name, LOG_CONS | LOG_PID, LOG_USER);
}

void log_write(int priority, const char* const message)
{
	syslog(priority, "%s", message);
}

void log_close()
{
	closelog();
}
*/

static void err_doit(int , int, const char *, va_list);

void
err_ret(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
}

void
err_sys(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	exit(1);
}

void
err_cont(int error, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
}

void
err_exit(int error, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	exit(1);
}

void
err_dump(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, errno, fmt, ap);
	va_end(ap);
	abort();
	exit(1);
}

void
err_msg(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
}

void
err_quit(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, 0, fmt, ap);
	va_end(ap);
	exit(1);
}

static void
err_doit(int errnoflag, int error, const char *fmt, va_list ap)
{
	char buf[4096];

	vsnprintf(buf, 4096 - 1, fmt, ap);
	if (errnoflag) {
		snprintf(buf + strlen(buf), 4096 - strlen(buf) - 1, ": %s", strerror(error));
	}
	strcat(buf, "\n");
	fflush(stdout);
	fputs(buf, stderr);
	fflush(NULL);
}

static void log_doit(int, int, int, const char *, va_list ap);

int log_to_stderr;

void
log_open(const char *ident, int option, int facility)
{
	if (log_to_stderr == 0) {
		openlog(ident, option, facility);
	}
}

void
log_ret(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(1, errno, LOG_ERR, fmt, ap);
	va_end(ap);
}

void
log_sys(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(1, errno, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}

void
log_msg(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(0, 0, LOG_ERR, fmt, ap);
	va_end(ap);
}

void
log_quit(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(0, 0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}

void
log_exit(int error, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	log_doit(1, error, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(2);
}

static void
log_doit(int errnoflag, int error, int priority, const char *fmt, va_list ap)
{
	char buf[4096];

	vsnprintf(buf, 4096 - 1, fmt, ap);
	if (errnoflag) {
		snprintf(buf + strlen(buf), 4096 - strlen(buf) - 1, ": %s", strerror(error));
	}
	strcat(buf, "\n");
	if (log_to_stderr) {
		fflush(stdout);
		fputs(buf, stderr);
		fflush(stderr);
	} else {
		syslog(priority, "%s", buf);
	}
}

void
daemonize(const char *cmd)
{
	int i, fd0, fd1, fd2;
	pid_t pid;
	struct rlimit rl;
	struct sigaction sa;

	umask(0);

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
		err_quit("%s : can't get file limit.\n", cmd);
	}

	if ((pid == fork()) < 0) {
		err_quit("%s : can't fork.\n", cmd);
	} else if (pid != 0) {
		exit(0);
	}
	setsid();

	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGHUP, &sa, NULL) < 0) {
		err_quit("%s : can't ignore SIGHUP.\n", cmd);
	}
	if ((pid = fork()) < 0) {
		err_quit("%s : can't fork.\n", cmd);
	} else if (pid != 0) {
		exit(0);
	}

	if (chdir("/") < 0) {
		err_quit("%s : can't change directory to /.\n", cmd);
	}

	if (rl.rlim_max == RLIM_INFINITY) {
		rl.rlim_max = 1024;
	}
	for (i = 0; i < rl.rlim_max; i++) {
		close(i);
	}

	fd0 = open("/dev/null", O_RDWR);
	fd1 = dup(0);
	fd2 = dup(0);

	openlog(cmd, LOG_CONS, LOG_DAEMON);
	if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
		syslog(LOG_ERR, "unexpected file descripters %d %d %d", fd0, fd1, fd2);
		exit(1);
	}
}

int
main(int argc, char *argv[])
{
	char *cmd;
	cmd = argv[0];

	daemonize(cmd);
}
