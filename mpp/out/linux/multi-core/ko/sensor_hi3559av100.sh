#!/bin/sh

####################Variables Definition##########################


SNS_TYPE0=NULL;           	# sensor type
SNS_TYPE1=NULL;            	# sensor type
SNS_TYPE2=NULL;           	# sensor type
SNS_TYPE3=NULL;            	# sensor type
SNS_TYPE4=NULL;           	# sensor type
SNS_TYPE5=NULL;            	# sensor type

##################################################################

report_error()
{
	echo "******* Error: There's something wrong, please check! *****"
	exit 1
}

#i2c0 -> sensor
i2c0_pin_mux()
{
	himm 0x1F000068 0x000014f3
	himm 0x1F00006C 0x000014f3
	echo "============i2c0_pin_mux done============="
}

#spi0 -> sensor
spi0_pin_mux()
{
	himm 0x1F000068 0x000004f1
	himm 0x1F00006c 0x000004f1
	himm 0x1F000070 0x000014f1
	himm 0x1F000074 0x000004f1
	himm 0x1F000078 0x000004f1
	echo "============spi0_pin_mux done============="
}

#spi1 -> sensor
spi1_pin_mux()
{
	himm 0x1F00007c 0x000004f1
	himm 0x1F000080 0x000004f1
	himm 0x1F000084 0x000014f1
	himm 0x1F000088 0x000004f1
	himm 0x1F00008c 0x000004f1
	echo "============spi1_pin_mux done============="
}

vi_bt1120_0_mode()
{
	himm 0x1F001084 0x000014f3
	himm 0x1F0010A4 0x000014f3
	himm 0x1F0010A0 0x000014f3
	himm 0x1F00109c 0x000014f3
	himm 0x1F001098 0x000014f3
	himm 0x1F001094 0x000014f3
	himm 0x1F001090 0x000014f3
	himm 0x1F00108c 0x000014f3
	himm 0x1F001088 0x000014f3
	himm 0x1F0010c4 0x000014f3
	himm 0x1F0010c0 0x000014f3
	himm 0x1F0010bc 0x000014f3
	himm 0x1F0010b8 0x000014f3
	himm 0x1F0010b4 0x000014f3
	himm 0x1F0010b0 0x000014f3
	himm 0x1F0010ac 0x000014f3
	himm 0x1F0010a8 0x000014f3
echo "============vi_bt1120_0_mux done============="
}
insert_sns()
{
	local sns_0_1_crg=0;
	local sns_2_3_crg=0;
	local sns_4_5_crg=0;

	case $SNS_TYPE0 in
		imx377)
			#himm 0x12030130 0x0;		#mipi mode
			#himm 0x120100f4 0xff3fff;	#core/cfg rstq
			#himm 0x120100f4 0xff0000;
			himm 0x12010114 0x00141414;	#24MHz
			;;
		imx334)
			himm 0x12030098 0x0;       # vi port work_mode¡êo mipi
			himm 0x1203009c 0x0;       # mipi work_mode: mipi
			#himm 0x120100f4 0x7ffff;   # core rstq
			#himm 0x120100f4 0x7fe00;
			himm 0x12010114 0x74747474;
			himm 0x12010114 0x14141414; # 3:0 -- c=12, b=25, a=27, 9=36, 8=37.125, 6=32.4, 4=24, 2=54, 1=72, 0=74.25MHz
			echo "============cfg sensor imx334============"
			;;
		imx477)
			#himm 0x111f0000 0x0000000;	#i2c mode
			#himm 0x111f1000 0x0000000;	#input mode ->mipi
			#himm 0x111f1004 0x00ff0000;	#VI PT clk
			#himm 0x111f1008 0x00000022;	#VI bus and ppc clk
			#himm 0x111f2008 0x000000ff;	#ISP clk
			#himm 0x111f2000 0x0;		#ISP CORE unreset
			#himm 0x111f2004 0x0;		#ISP CFG unreset
			#himm 0x113e0000 0x00000202;	#VIPROC1 online
			#himm 0x113f0000 0x00000202;	#VIPROC0 online
			#himm 0x113e0000 0x00000002;     #VIPROC1 offline
			#himm 0x113f0000 0x00000002;     #VIPROC0 offline
			#himm 0x111e1000 0x0;       	# mipi_mode
			#himm 0x111e1004 0xff;      	# mipi core/cfg rstq
			#himm 0x111e1004 0xff0000; 	# mipi clk
			#himm 0x111e3000 0x00049000; # 7=37.125, 6=3, 5=27, 4=24, 3=18, 2=13.5, 1=12, 0=6. here 2=13.5;
			himm 0x12030098 0x0;       # vi port work_mode¡êo mipi
			himm 0x1203009c 0x0;       # mipi work_mode: mipi
			#himm 0x120100f4 0x7ffff;   # core reset + pix0~7 clk_en
			#himm 0x120100f4 0x7fe00;   # core unreset
			himm 0x12010114 0x74747474;
			himm 0x12010114 0x14141414; # 3:0 -- c=12, b=25, a=27, 9=36, 8=37.125, 6=32.4, 4=24, 2=54, 1=72, 0=74.25MHz
			echo "============cfg sensor 477============"
			;;
		imx290)
			#himm 0x111f0000 0x0000000;	#i2c mode
			#himm 0x111f1000 0x0000000;	#input mode ->mipi
			#himm 0x111f1004 0x00ff0000;	#VI PT clk
			#himm 0x111f1008 0x00000022;	#VI bus and ppc clk
			#himm 0x111f2008 0x000000ff;	#ISP clk
			#himm 0x111f2000 0x0;		#ISP CORE unreset
			#himm 0x111f2004 0x0;		#ISP CFG unreset
			#himm 0x113e0000 0x00000202;	#VIPROC1 online
			#himm 0x113f0000 0x00000202; 	#VIPROC0 online
			#himm 0x113e0000 0x00000002;     #VIPROC1 offline
			#himm 0x113f0000 0x00000002;     #VIPROC0 offline
			#himm 0x111e1000 0x0;        	# mipi_mode
			#himm 0x111e1004 0xff;      	# mipi core/cfg rstq
			#himm 0x111e1004 0xff0000; 	# mipi clk
			#himm 0x111e3000 0x00049300; # 7=37.125, 6=3, 5=27, 4=24, 3=18, 2=13.5, 1=12, 0=6. here 3=18;
			himm 0x12030098 0x0;       # vi port work_mode¡êo mipi
			himm 0x1203009c 0x0;       # mipi work_mode: mipi
			#himm 0x120100f4 0x7ffff;   # core rstq + pix0~7 clk_en
			#himm 0x120100f4 0x7fe00;   # core unreset
			himm 0x12010114 0x78787878;# sensor clk?¡é sensor clk_en ?¡ésensor reset
			himm 0x12010114 0x18181818;# sensor clk?¡é sensor clk_en ?¡ésensor unreset 3:0 -- c=12, b=25, a=27, 9=36, 8=37.125, 6=32.4, 4=24, 2=54, 1=72, 0=74.25MHz
			echo "============cfg sensor 290============"
			;;
		imx290_slave)
			himm 0x12030098 0x0;       # vi port work_mode¡êo mipi
			himm 0x1203009c 0x0;       # mipi work_mode: mipi
			#himm 0x120100f4 0x7ffff;   # core rstq + pix0~7 clk_en
			#himm 0x120100f4 0x7fe00;   # core unreset
			himm 0x12010114 0x78787878;# sensor clk?¡é sensor clk_en ?¡ésensor reset
			himm 0x12010114 0x10101010;# sensor clk?¡é sensor clk_en ?¡ésensor unreset 3:0 -- c=12, b=25, a=27, 9=36, 8=37.125, 6=32.4, 4=24, 2=54, 1=72, 0=74.25MHz
			echo "============cfg sensor 290_slave============"
			;;
		imx299)
			i2c0_pin_mux;
			himm 0x12030098 0x0;       # vi port work_mode¡êo mipi
			himm 0x1203009c 0x0;       # mipi work_mode: mipi
			#himm 0x120100f4 0x7ffff;   # core rstq + pix0~7 clk_en
			#himm 0x120100f4 0x7fe00;   # core unreset
			himm 0x12010114 0x74747474;# sensor clk?¡é sensor clk_en ?¡ésensor reset
			himm 0x12010114 0x14141414;# sensor clk?¡é sensor clk_en ?¡ésensor unreset 3:0 -- c=12, b=25, a=27, 9=36, 8=37.125, 6=32.4, 4=24, 2=54, 1=72, 0=74.25MHz
			echo "============cfg sensor 299============"
			;;
		imx299_slvs)
			spi0_pin_mux;
			himm 0x12030098 0x0;       # vi port work_mode¡êo mipi
			himm 0x1203009c 0x0;       # mipi work_mode: mipi
			#himm 0x120100f4 0x7ffff;   # core rstq + pix0~7 clk_en
			#himm 0x120100f4 0x7fe00;   # core unreset
			himm 0x12010114 0x71717171;# sensor clk?¡é sensor clk_en ?¡ésensor reset
			himm 0x12010114 0x11111111;# sensor clk?¡é sensor clk_en ?¡ésensor unreset 3:0 -- c=12, b=25, a=27, 9=36, 8=37.125, 6=32.4, 4=24, 2=54, 1=72, 0=74.25MHz
			echo "============cfg sensor 299_slvs============"
			;;
		imx277_slvs)
			spi0_pin_mux;
			himm 0x12030098 0x0;       # vi port work_mode¡êo mipi
			himm 0x1203009c 0x0;       # mipi work_mode: mipi
			#himm 0x120100f4 0x7ffff;   # core rstq + pix0~7 clk_en
			#himm 0x120100f4 0x7fe00;   # core unreset
			himm 0x12010114 0x71717171;# sensor clk?¡é sensor clk_en ?¡ésensor reset
			himm 0x12010114 0x11111111;# sensor clk?¡é sensor clk_en ?¡ésensor unreset 3:0 -- c=12, b=25, a=27, 9=36, 8=37.125, 6=32.4, 4=24, 2=54, 1=72, 0=74.25MHz
			echo "============cfg sensor 277_slvs============"
			;;
		cmv50000)
			#himm 0x12030130 0x555;        # mipi_mode
            		#himm 0x120100f4 0xff3fff;     # core/cfg rstq
            		#himm 0x120100f4 0xff0000;
			#himm 0x12010114 0x00343434;
            		himm 0x12010114 0x00141414;    #24MHz
			spi0_pin_mux;
			spi1_pin_mux;
			;;
		imx117)
			spi0_pin_mux;
			himm 0x12030098 0x0;       # vi port work_mode¡êo mipi
			himm 0x1203009c 0x1;       # mipi work_mode: mipi
			#himm 0x120100f4 0x7ffff;   # core rstq + pix0~7 clk_en
			#himm 0x120100f4 0x7fe00;   # core unreset
			himm 0x12010114 0x71717171;# sensor clk?¡é sensor clk_en ?¡ésensor reset
			himm 0x12010114 0x11111111;# sensor clk?¡é sensor clk_en ?¡ésensor unreset 3:0 -- c=12, b=25, a=27, 9=36, 8=37.125, 6=32.4, 4=24, 2=54, 1=72, 0=74.25MHz
			;;
		bt1120)
			vi_bt1120_0_mode;
			himm 0x12010104 0x00c28e00;
			;;
	esac

	echo "==== Your input Sensor0 type is $SNS_TYPE0 ===="
	echo "==== Your input Sensor1 type is $SNS_TYPE1 ===="
	echo "==== Your input Sensor2 type is $SNS_TYPE2 ===="
	echo "==== Your input Sensor3 type is $SNS_TYPE3 ===="
	echo "==== Your input Sensor4 type is $SNS_TYPE4 ===="
	echo "==== Your input Sensor5 type is $SNS_TYPE5 ===="
}

load_usage()
{
	echo "Usage:  ./sensor_hi3559av100es.sh [sensor_name]"
	echo "    -sensor0 sensor_name      config sensor type [default: imx377]"
	echo "    -sensor1 sensor_name      config sensor type [default: imx377]"
	echo "    -h                       help information"
	echo -e "Available sensors: imx377, imx477, imx299, imx299_slvs, cmv50000\n"
	echo -e "Sensor[n] must be same as sensor[n+1],n=0 2 4\n"
	echo -e "for example: ./sensor_hi3559av100es.sh -sensor0 imx377 -sensor1 imx377\n"
}

######################parse arg###################################
b_arg_sensor0=0
b_arg_sensor1=0
b_arg_sensor2=0
b_arg_sensor3=0
b_arg_sensor4=0
b_arg_sensor5=0

for arg in $@
do
	if [ $b_arg_sensor0 -eq 1 ] ; then
		b_arg_sensor0=0;
		SNS_TYPE0=$arg;
	fi
	if [ $b_arg_sensor1 -eq 1 ] ; then
		b_arg_sensor1=0;
		SNS_TYPE1=$arg;
	fi
	if [ $b_arg_sensor2 -eq 1 ] ; then
		b_arg_sensor2=0;
		SNS_TYPE2=$arg;
	fi
	if [ $b_arg_sensor3 -eq 1 ] ; then
		b_arg_sensor3=0;
		SNS_TYPE3=$arg;
	fi
	if [ $b_arg_sensor4 -eq 1 ] ; then
		b_arg_sensor4=0;
		SNS_TYPE4=$arg;
	fi
	if [ $b_arg_sensor5 -eq 1 ] ; then
		b_arg_sensor5=0;
		SNS_TYPE5=$arg;
	fi

	case $arg in
		"-sensor0")
			b_arg_sensor0=1;
			;;
		"-sensor1")
			b_arg_sensor1=1;
			;;
		"-sensor2")
			b_arg_sensor2=1;
			;;
		"-sensor3")
			b_arg_sensor3=1;
			;;
		"-sensor4")
			b_arg_sensor4=1;
			;;
		"-sensor5")
			b_arg_sensor5=1;
			;;
		"-h")
			load_usage;
			;;
	esac
done

if [ $# -lt 1 ]; then
    load_usage;
    exit 0;
fi

insert_sns;
