#!/bin/sh

# This is a sample, you should rewrite it according to your chip #
# You can configure your pinmux for the application here!

#VICAP default setting is VI
sensor_pin_mux()
{
	#CLK_A CLK_B 
	himm 0x1F000000 0x000004f1
	himm 0x1F000004 0x000004f1
	
	himm 0x1F000008 0x000004f1
	himm 0x1F00000C 0x000004f1
	
	himm 0x1F000010 0x000004f1
	himm 0x1F000014 0x000004f1
	
	himm 0x1F000018 0x000004f1
	himm 0x1F00001C 0x000004f1
	
	
	#RST
	himm 0x1F000020 0x000004f1
	himm 0x1F000024 0x000004f1
	himm 0x1F000028 0x000004f1
	himm 0x1F00002C 0x000004f1
	echo "============vicap_pin_mux done============="			
}

slave_pin_mux()
{
	#VS HS
	himm 0x1F000030 0x000004f1
	himm 0x1F000034 0x000004f1
	                       
	himm 0x1F000038 0x000004f1
	himm 0x1F00003C 0x000004f1
	                       
	himm 0x1F000040 0x000004f1
	himm 0x1F000044 0x000004f1
	                       
	himm 0x1F000048 0x000004f1
	himm 0x1F00004C 0x000004f1
	                       
	#VSOUT                 
	himm 0x1F000050 0x000004f1
	himm 0x1F000054 0x000004f1
	himm 0x1F000058 0x000004f1
	himm 0x1F00005C 0x000004f1
	echo "============slave_pin_mux done============="	
}



#i2c0 -> sensor
i2c0_pin_mux()
{
	#SCL SDA
	himm 0x1F000068 0x000014f3
	himm 0x1F00006C 0x000014f3

	echo "============i2c0_pin_mux done============="
}

#i2c1 -> sensor
i2c1_pin_mux()
{
	#SCL SDA
	himm 0x1F00007C 0x000014f3
	himm 0x1F000080 0x000014f3

	echo "============i2c1_pin_mux done============="
}


#i2c2 -> sensor
i2c2_pin_mux()
{
	#SCL SDA
	himm 0x1F000090 0x000014f3
	himm 0x1F000094 0x000014f3

	echo "============i2c2_pin_mux done============="
}

#i2c3 -> sensor
i2c3_pin_mux()
{
	#SCL SDA
	himm 0x1F0000A4 0x000014f3
	himm 0x1F0000A8 0x000014f3

	echo "============i2c3_pin_mux done============="
}


#i2c4 -> sensor
i2c4_pin_mux()
{
	#SCL SDA                
	himm 0x1F000074 0x000014f3
	himm 0x1F000078 0x000014f3
 
	echo "============i2c4_pin_mux done============="
}


#i2c5 -> sensor
i2c5_pin_mux()
{
	#SCL SDA
	himm 0x1F000088 0x000014f3
	himm 0x1F00008C 0x000014f3

	echo "============i2c5_pin_mux done============="
}

#i2c6 -> sensor
i2c6_pin_mux()
{
	#SCL SDA
	himm 0x1F00009C 0x000014f3
	himm 0x1F0000A0 0x000014f3


	echo "============i2c6_pin_mux done============="
}

#i2c7 -> sensor
i2c7_pin_mux()
{
	#SCL SDA
	himm 0x1F0000B0 0x000014f3
	himm 0x1F0000B4 0x000014f3

	echo "============i2c7_pin_mux done============="
}

#i2c8 -> sensor
i2c8_pin_mux()
{
	#SCL SDA
	himm 0x1F0010C8 0x000014f2
	himm 0x1F0010CC 0x000014f2

	echo "============i2c8_pin_mux done============="
}

#i2c9 -> sensor
i2c9_pin_mux()
{
	#SCL SDA
	himm 0x1F0010D0 0x000014f2
	himm 0x1F0010D4 0x000014f2

	echo "============i2c9_pin_mux done============="
}

#i2c10 -> sensor
i2c10_pin_mux()
{
	#SCL SDA
	himm 0x1F0010DC 0x000014f2
	himm 0x1F0010E0 0x000014f2

	echo "============i2c10_pin_mux done============="
}

#i2c11 -> 9022
i2c11_pin_mux()
{
	himm 0x1F0010F4 0x00001C00
	himm 0x1F0010F0 0x00001C00
	echo "============i2c11_pin_mux done============="
}

vo_bt1120_mode()
{
	himm 0x1F001084 0x000000f1
	himm 0x1F001088 0x000004f1
	himm 0x1F00108C 0x000004f1
	himm 0x1F001090 0x000004f1
	himm 0x1F001094 0x000004f1
	himm 0x1F001098 0x000004f1
	himm 0x1F00109C 0x000004f1
	himm 0x1F0010A0 0x000004f1
	himm 0x1F0010A4 0x000004f1
	himm 0x1F0010A8 0x000004f1
	himm 0x1F0010AC 0x000004f1
	himm 0x1F0010B0 0x000004f1
	himm 0x1F0010B4 0x000004f1
	himm 0x1F0010B8 0x000004f1
	himm 0x1F0010BC 0x000004f1
	himm 0x1F0010C0 0x000004f1
	himm 0x1F0010C4 0x000004f1

	echo "============vo_output_mode done============="
}

vo_lcd_mode()
{
	himm 0x1F0010EC 0x000004f2
	himm 0x1F0010A4 0x000004f2
	himm 0x1F00109C 0x000004f2
	himm 0x1F0010E4 0x000004f2
	himm 0x1F001098 0x000004f2
	himm 0x1F001094 0x000004f2
	himm 0x1F001090 0x000004f2
	himm 0x1F00108C 0x000004f2
	himm 0x1F001088 0x000004f2
	himm 0x1F0010C4 0x000004f2
	himm 0x1F0010C0 0x000004f2
	himm 0x1F0010A8 0x000004f2
	himm 0x1F0010B8 0x000004f2
	himm 0x1F0010B4 0x000004f2
	himm 0x1F0010B0 0x000004f2
	himm 0x1F0010AC 0x000004f2
	himm 0x1F001008 0x000004f2
	himm 0x1F00100C 0x000004f2
	himm 0x1F001010 0x000004f2
	himm 0x1F001000 0x000004f2
	himm 0x1F001014 0x000004f2
	himm 0x1F001038 0x000004f2
	himm 0x1F00101C 0x000004f2
	himm 0x1F001030 0x000004f2
	himm 0x1F001020 0x000004f2
	himm 0x1F001024 0x000004f2
	himm 0x1F001028 0x000004f2
	himm 0x1F00102C 0x000004f2

	echo "============vo_output_mode done============="
}

vo_lcd8bit_mode()
{
        #lcd8bit 
	himm 0x1f001088 0x4f2
	himm 0x1f00108c 0x4f2
	himm 0x1f001090 0x4f2
	himm 0x1f001094 0x4f2
	himm 0x1f001098 0x4f2
	himm 0x1f00109c 0x4f2
	himm 0x1f0010a4 0x4f2
	himm 0x1f0010a8 0x4f2
	himm 0x1f0010c0 0x4f2
	himm 0x1f0010c4 0x4f2
	himm 0x1f0010e4 0x4f2
	himm 0x1f0010ec 0x4f2
	echo "============vo_lcd8bit_mode done============="
}

spi4_pin_mux()                                          
{                                                       
	himm 0x1f0010c8 0x1       #(GPIO16_0,SPI4_SCLK,     I2C8_SCL)  = (0x0,0x1,0x2)
	himm 0x1f0010cc 0x1       #(GPIO16_1,SPI4_SDO ,     I2C8_SCA)  = (0x0,0x1,0x2)
	himm 0x1f0010d0 0x1       #(GPIO16_2,SPI4_SDI ,     I2C9_SCL)  = (0x0,0x1,0x2)
	himm 0x1f0010d4 0x1       #(GPIO16_3,SPI4_CSN0,     I2C9_SDA)  = (0x0,0x1,0x2)
	himm 0x1f0010d8 0x1       #(GPIO16_4,SPI4_CSN1)                = (0x0,0x1)
	himm 0x1f0010dc 0x1       #(GPIO16_5,SPI4_CSN2,     I2C10_SCL) = (0x0,0x1,0x2)
	himm 0x1f0010e0 0x1       #(GPIO16_6,SPI4_CSN3,     I2C10_SDA) = (0x0,0x1,0x2)
	echo "============spi4_pin_mux done============="   
}

hmdi_pin_mode()
{
	himm 0x1F0000C4 0x00001400
	himm 0x1F0000C8 0x00001400
	himm 0x1F0000CC 0x00001400
	himm 0x1F0000D0 0x00001400
}

i2s_pin_mux()
{
	himm 0x1F0000FC 0x00000ef2
	himm 0x1F000100 0x00001ef2
	himm 0x1F000104 0x00001ef2
	himm 0x1F000108 0x00000ef2
	himm 0x1F00010C 0x00001ef2

	echo "============i2s_pin_mux done============="
}


echo "==pinmux_Hi3559AV100_asic=="

   sensor_pin_mux;
   slave_pin_mux;
   
   #sensor
   i2c0_pin_mux;
   i2c1_pin_mux;
   i2c2_pin_mux;
   i2c3_pin_mux;
   i2c4_pin_mux;
   i2c5_pin_mux;
   i2c6_pin_mux;
   i2c7_pin_mux;
   
   
   #9022
   i2c11_pin_mux;

   #bt1120
   vo_bt1120_mode;

   #lcd_24bit
   #vo_lcd_mode

   #lcd_8bit
   #spi4_pin_mux
   #vo_lcd8bit_mode

   hmdi_pin_mode;
   
   i2s_pin_mux;
   