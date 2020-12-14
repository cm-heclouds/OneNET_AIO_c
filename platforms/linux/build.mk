obj-y += osl_linux.o
obj-y += tcp_linux.o
obj-y += time_linux.o
ifdef CONFIG_TM_COAP
obj-y += udp_linux.o
obj-y += task_linux.o
endif
obj-$(CONFIG_IOT_NBIOT) += uart_linux.o
