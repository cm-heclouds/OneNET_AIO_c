obj-y += osl_linux.o
obj-y += time_linux.o

ifdef CONFIG_MQTT
obj-y += tcp_linux.o
endif
ifdef CONFIG_HTTP
obj-y += tcp_linux.o
endif
ifdef CONFIG_IOT_EDP
obj-y += tcp_linux.o
endif

ifdef CONFIG_COAP
obj-y += udp_linux.o
endif

obj-$(CONFIG_MOD_ADAPTOR) += uart_linux.o
