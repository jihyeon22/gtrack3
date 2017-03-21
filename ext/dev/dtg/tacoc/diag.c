#include <stdio.h>

#include <wrapper/dtg_log.h>
#include <wrapper/dtg_atcmd.h>
#include <diag.h>

#define	BUFF_SIZE			1024

int create_diag_info(int flag)
{
	int ret;
	char buff[BUFF_SIZE] = { 0 };
	char *newline = "\n\n";
	FILE *fp1, *fp2;

	fp2 = fopen("/tmp/diag.txt", "w");
	if (fp2 == NULL) {
		perror("/tmp/diag.txt fopen fail");
		return -1;
	}

	/* System */
	if (flag & LINUX_VERSION) {
		fp1 = popen("cat /proc/version", "r");
		if (fp1 == NULL) {
			return -1;
		}

		while (fgets(buff, sizeof(buff) - 1, fp1)) {
			fwrite(buff, 1, strlen(buff), fp2);
		}

		pclose(fp1);
	}

	memset(buff, 0, sizeof(buff));
	fwrite(newline, 1, strlen(newline), fp2);

	if (flag & DMESG) {
		fp1 = popen("dmesg | tail -n 128", "r");
		if (fp1 == NULL) {
			return -1;
		}

		while (fgets(buff, sizeof(buff) - 1, fp1)) {
			fwrite(buff, 1, strlen(buff), fp2);
		}

		pclose(fp1);
	}
	
	memset(buff, 0, sizeof(buff));
	fwrite(newline, 1, strlen(newline), fp2);

	if (flag & PS) {
		fp1 = popen("ps", "r");
		if (fp1 == NULL) {
			return -1;
		}

		while (fgets(buff, sizeof(buff) - 1, fp1)) {
			fwrite(buff, 1, strlen(buff), fp2);
		}

		pclose(fp1);
	}
	
	memset(buff, 0, sizeof(buff));
	fwrite(newline, 1, strlen(newline), fp2);

	if (flag & MEMINFO) {
		fp1 = popen("cat /proc/meminfo", "r");
		if (fp1 == NULL) {
			return -1;
		}

		while (fgets(buff, sizeof(buff) - 1, fp1)) {
			fwrite(buff, 1, strlen(buff), fp2);
		}

		pclose(fp1);
	}
	
	memset(buff, 0, sizeof(buff));
	fwrite(newline, 1, strlen(newline), fp2);

	if (flag & FILESYSTEM) {
		fp1 = popen("df -h", "r");
		if (fp1 == NULL) {
			return -1;
		}

		while (fgets(buff, sizeof(buff) - 1, fp1)) {
			fwrite(buff, 1, strlen(buff), fp2);
		}

		pclose(fp1);
	}
	
	memset(buff, 0, sizeof(buff));
	fwrite(newline, 1, strlen(newline), fp2);

	if (flag & UPTIME) {
		fp1 = popen("uptime", "r");
		if (fp1 == NULL) {
			return -1;
		}

		while (fgets(buff, sizeof(buff) - 1, fp1)) {
			fwrite(buff, 1, strlen(buff), fp2);
		}

		pclose(fp1);
	}

	/* Modem */

	memset(buff, 0, sizeof(buff));
	fwrite(newline, 1, strlen(newline), fp2);

	if (flag & DBGSCRN) {
		
		if (ret < atcmd_get_debugscrn(buff, sizeof(buff) - 1)) {
			DTG_LOGE("atcmd_get_debugscrn failure");
		} else {
			fwrite(buff, 1, strlen(buff), fp2);
		}
	}

	memset(buff, 0, sizeof(buff));
	fwrite(newline, 1, strlen(newline), fp2);

	/* Application */

	sprintf(buff, "PACKAGE: %d\n"
				  "VERSION: %d.%02d\n"
				  "SERVER: %s:%d\n"
				  "INTERVAL: %d\n"
				  "Error Reporting: %s\n",
				  get_package_type(),
				  get_package_major_version(),
				  get_package_minor_version(),
				  get_server_ip_addr(),
				  get_server_port(),
				  get_interval(),
				  get_err_report_phonenum() ? get_err_report_phonenum() : "None");
	fwrite(buff, 1, strlen(buff), fp2);

	fclose(fp2);
	
	return 1;
}
