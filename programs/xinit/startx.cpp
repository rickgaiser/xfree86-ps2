XCOMM!/bin/sh
XCOMM
XCOMM (c) 1999 Red Hat Software, Inc.

bindir=BINDIR

userclientrc=$HOME/.xinitrc
userserverrc=$HOME/.xserverrc
sysclientrc=/etc/X11/xinit/xinitrc
sysserverrc=/etc/X11/xinit/xserverrc
clientargs=""
serverargs=""

if [ -f $userclientrc ]; then
    clientargs=$userclientrc
else if [ -f $sysclientrc ]; then
    clientargs=$sysclientrc
fi
fi

if [ -f $userserverrc ]; then
    serverargs=$userserverrc
else if [ -f $sysserverrc ]; then
    serverargs=$sysserverrc
fi
fi

display=:0
whoseargs="client"
while [ "x$1" != "x" ]; do
    case "$1" in
	/''*|\.*)	if [ "$whoseargs" = "client" ]; then
		    if [ "x$clientargs" = x ]; then
			clientargs="$1"
		    else
			clientargs="$clientargs $1"
		    fi
		else
		    if [ "x$serverargs" = x ]; then
			serverargs="$1"
		    else
			serverargs="$serverargs $1"
		    fi
		fi ;;
	--)	whoseargs="server" ;;
	*)	if [ "$whoseargs" = "client" ]; then
		    clientargs="$clientargs $1"
		else
    		    case "$1" in
		        :[0-9]) display="$1"
		        ;;
                        *) serverargs="$serverargs $1"
			;;
		    esac
		fi ;;
    esac
    shift
done

XCOMM set up default Xauth info for this machine
mcookie=`mcookie`
serverargs="$serverargs -auth $HOME/.Xauthority"
xauth add $display . $mcookie
xauth add `hostname -f`$display . $mcookie

xinit $clientargs -- $display $serverargs

XCOMM various machines need special cleaning up,
XCOMM which should be done here

#ifdef macII
Xrepair
screenrestore
#endif

#if defined(sun) && !defined(i386)
kbd_mode -a
#endif
