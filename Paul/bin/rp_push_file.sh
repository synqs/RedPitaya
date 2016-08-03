#!/bin/bash
if [ $# -lt 4 ]
then
	echo "usage: rp_push_file password hostname destination source"
	exit
fi
SRC=$4
PSWD=$1
HOST=$2
DEST=$3
while [ $# -gt 4 ]
do
	SRC+=" "$5
	shift
done
echo "stop NGINX and make filesystem writable"
sshpass -p $PSWD ssh root@$HOST 'source /etc/profile; systemctl stop redpitaya_nginx.service; rw; exit'
echo "pushing "$SRC" to "$DEST
sshpass -p $PSWD scp $SRC root@$HOST:$DEST
echo "make filesystem read only and restart NGINX"
sshpass -p $PSWD ssh root@$HOST 'source /etc/profile; ro; systemctl start redpitaya_nginx.service; exit'
