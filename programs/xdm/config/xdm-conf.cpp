! $XFree86: xc/programs/xdm/config/xdm-conf.cpp,v 1.1.1.2.4.2 1999/10/12 18:33:29 hohndel Exp $
!
! $XConsortium: xdm-conf.cpp /main/3 1996/01/15 15:17:26 gildea $
DisplayManager.errorLogFile:	/var/log/xdm-error.log
DisplayManager.pidFile:		/var/run/xdm.pid
DisplayManager.keyFile:		/etc/X11/xdm/xdm-keys
DisplayManager.servers:		/etc/X11/xdm/Xservers
DisplayManager.accessFile:	/etc/X11/xdm/Xaccess
! All displays should use authorization, but we cannot be sure
! X terminals will be configured that way, so by default
! use authorization only for local displays :0, :1, etc.
DisplayManager._0.authorize:	true
DisplayManager._1.authorize:	true
! The following three resources set up display :0 as the console.
DisplayManager._0.setup:	/etc/X11/xdm/Xsetup_0
DisplayManager._0.startup:	/etc/X11/xdm/GiveConsole
DisplayManager._0.reset:	/etc/X11/xdm/TakeConsole
!
DisplayManager*resources:	/etc/X11/xdm/Xresources
DisplayManager*session:		/etc/X11/xinit/Xsession
DisplayManager*authComplain:	false
! SECURITY: do not listen for XDMCP or Chooser requests
! Comment out this line if you want to manage X terminals with xdm
DisplayManager.requestPort:	0
