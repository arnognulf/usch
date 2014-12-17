#
# Regular cron jobs for the usch package
#
0 4	* * *	root	[ -x /usr/bin/usch_maintenance ] && /usr/bin/usch_maintenance
