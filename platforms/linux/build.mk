obj-y += osl_linux.o
obj-y += time_linux.o
ifdef CONFIG_TM_COAP
obj-y += udp_linux.o
else
ifndef CONFIG_IOT_NBIOT
obj-y += tcp_linux.o
endif
endif
obj-$(CONFIG_MOD_ADAPTOR) += uart_linux.o
