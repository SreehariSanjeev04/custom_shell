#ifndef FOCUS_MODE_H
#define FOCUS_MODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define HOSTS_FILE "/etc/hosts"
#define BACKUP_FILE "/etc/hosts.backup"
#define BLOCKED_SITES 1

void enable_focus_mode();

void disable_focus_mode();

void focus_mode_status();

#endif 