#!/bin/sh
# **************************************************************************
# * THIS FILE IS DEPRECATED and is not designed to work on the VM/370 CE ***
# * Going forward installation is a responsibility of the distribution not *
# * of the package provider                                                *
# **************************************************************************
#
# Install YATA on CMS
#


# Exit if there is an error
set -e

# IPL
herccontrol -v
herccontrol "ipl 141" -w "USER DSC LOGOFF AS AUTOLOG1"

# LOGON MAINT AND READ TAPE
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon maint cpcms" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"
herccontrol "devinit 480 io/yatabin.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to maint as 181" -w "TAPE 480 ATTACH TO MAINT"
herccontrol "/access 19e c" -w "^Ready;"
herccontrol "/tape load * * c" -w "^Ready;"
herccontrol "/rename yata module c1 = = c2"  -w "^Ready;"
herccontrol "/detach 181" -w "^Ready;"
herccontrol "/release c"  -w "^Ready;"
herccontrol "/ipl 190" -w "^CMS VERSION"
herccontrol "/savesys cms" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"
herccontrol "/logoff" -w "^VM/370 Online"

# LOGON CMSUSER
herccontrol "/logon cmsuser cmsuser" -w "^CMS VERSION"
herccontrol "/" -w "^Ready;"

# Sanity test
herccontrol "/yata -v" -w "^Ready;"

# LOGOFF
herccontrol "/logoff" -w "^VM/370 Online"

# SHUTDOWN
herccontrol "/logon operator operator" -w "RECONNECTED AT"
herccontrol "/shutdown" -w "^HHCCP011I"
