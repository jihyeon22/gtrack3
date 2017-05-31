#! /bin/sh
scrub_check=`cat /etc/btime | grep scrub`
fwexit=`cat /data/kt_dms/cfg/fwexitflag`
ftp_addr="virtual.mdstec.com/home/image_diff/tl500s/"
ftp_id="teladin_ftp"
ftp_pass="teladin.fota"

fota_start=0;

echo "fwexitflag set :"$fwexit

#data IF check.
data_if=`ifconfig | grep rmnet_data0`
if [ "$fwexit" != "1" ]; then
	if [ "$data_if" == "" ]; then
		c=1;
		while [ $c == 1 ];
		do
			sleep 5
			data_if=`ifconfig | grep rmnet_data0`
			if [ "$data_if" != "" ]; then
				c=2;
			fi
		done
	else
		echo "Netowrk IF OK"
	fi
else
	sleep 10
fi	

sw_ver=`/etc/factory/atcmd.sh ati | grep "Revision" | tr -d "\n\r"`
#Scrub Fail check.
if [ "$scrub_check" == "scrub_Fail" ]; then
	echo "Fota Updated fail!!!"
	exit
fi

#for MDS FW download
if [ "$sw_ver" == "Revision: TL500S_1.1.0 [Feb 21 2017 10:06:35]" ]; then
	echo "Old Version"

	if [ "$fwexit" == "1" ]; then
		echo "fwexitflag initialized!"
		echo 0 > /data/kt_dms/cfg/fwexitflag
		echo "scrub_Fail" >> /etc/btime
		reboot
		exit	
	fi

	wget ftp://$ftp_id:$ftp_pass@$ftp_addr'tl500s_mds.tar' -O /data/tl500s_mds.tar
	FILESIZE=$(stat -c%s "/data/tl500s_mds.tar")
	if [ $FILESIZE != 18406412 ]; then
		echo "FTP Download Re-try!!"
		rm -rf /data/tl500s_mds.tar
		wget $ftp_addr'tl500s_mds.tar' -O /data/tl500s_mds.tar
		FILESIZE=$(stat -c%s "/data/tl500s_mds.tar")
	fi
	if [ $FILESIZE == 18406412 ]; then
		fota_start=1;
		echo "FTP Download OK!!"
	fi	
fi

#new version.
if [ "$sw_ver" == "Revision: TL500S_1.1.0 [May 11 2017 11:41:26]" ]; then
	echo "scrub_Success" `date` >> /etc/btime
	if [ "$fwexit" == "1" ]; then
		echo "fwexitflag initialized!"
		echo 0 > /data/kt_dms/cfg/fwexitflag
		reboot
		exit
	fi
	
	echo "New version : scrub[$scrub_check]"
	if [ "$scrub_check" == "" ]; then
		serial=`/etc/factory/atcmd.sh AT\\$\\$FACTORY_SN? | grep '$$FACTORY_SN:' | awk -F' ' '{print $3}'`
		echo $serial
		echo "mv /data/tld_fota/dump_serial /data/tld_fota/$serial"
		mv /data/tld_fota/dump_serial /data/tld_fota/$serial
		echo $sw_ver >> /data/tld_fota/$serial
		date >> /data/tld_fota/$serial
		ftp_send=`/data/tld_fota/wput /data/tld_fota/$serial ftp://$ftp_id:$ftp_pass@$ftp_addr | grep FINISH`
		if [ "$ftp_send" == "" ]; then
			ftp_send=`/data/tld_fota/wput /data/tld_fota/$serial ftp://$ftp_id:$ftp_pass@$ftp_addr | grep FINISH`		
		fi
		if [ "$ftp_send" != "" ]; then
			echo $sw_ver >> /data/tld_fota/$serial
			date >> /data/tld_fota/$serial
		fi 
	fi
	exit
fi

# Setting for Update.
if [ $fota_start == 1 ]; then		
	mtdnum=`cat /proc/mtd | grep recoveryfs | awk -F' ' '{print $1}' | tr -d "mtd" |tr -d ":"`
	/usr/sbin/ubiattach -m $mtdnum -d 3 /dev/ubi_ctrl
	ubi_check=`ubinfo | grep ubi3`
	if [ "$ubi_check" != "" ]; then
		/bin/mount -t ubifs ubi3_0 /mnt
		if [ -f /mnt/etc/init.d/kt_dms.tar.gz ]; then
			echo "re-install fota demon!!"
			rm -rf /kt_dms
			rm -rf /data/kt_dms
			tar -xvzf /mnt/etc/init.d/kt_dms.tar.gz -C /data/
			ln -s /data/kt_dms /kt_dms
			/bin/sed -i "4s/.*/\/data\/tld_fota\/tld_recovery.sh \&\n/g" /mnt/etc/init.d/kt_svc
			mv /mnt/etc/init.d/mds_conf.sh /mnt/etc/init.d/_del_mds_conf.sh
			mv /mnt/etc/init.d/mds_init.sh /mnt/etc/init.d/_del_mds_init.sh
		fi
		sync
		/bin/umount /mnt
		/usr/sbin/ubidetach -d 3
	fi
	
        chmod 777 /data/tl500s_mds.tar
        tar xvf /data/tl500s_mds.tar -C /data/
        chmod 777 /data/tld_fota/*
        rm -f /data/tl500s_mds.tar
        echo 1 > /kt_dms/cfg/fwexitflag
        mv -f /data/tld_fota/TL500S_Mds_diff.bin /data/kt_fota/TL500S_Mds_diff.bin
	if [ $fota_start == 2 ]; then
		/usr/bin/fota_test erase system_dup
		/usr/bin/fota_test write system_dup /data/tld_fota/mdm9607-sysfs-dup.ubi
		rm -rf /data/tld_fota/mdm9607-sysfs-dup.ubi
	fi 
        killall -9 QCMAP_ConnectionManager
        sleep 3
        rm -f /usr/bin/QCMAP_ConnectionManager
        mv -f /data/tld_fota/QCMAP_ConnectionManager /usr/bin/QCMAP_ConnectionManager
        sync
        sys_reboot recovery
fi	