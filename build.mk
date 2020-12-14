obj-y += platforms/
obj-y += onenet/
obj-y += common/

ifdef CONFIG_TM
obj-y += tools/
obj-y += protocols/
endif

ifdef CONFIG_IOT_MQTTS
obj-y += tools/
obj-y += protocols/
endif

ifdef CONFIG_IOT_NBIOT
obj-y += tools/
endif

ifdef CONFIG_IOT_MQTT
obj-y += protocols/
endif

obj-$(CONFIG_NETWORK_TLS) += security/

obj-$(CONFIG_BUILD_EXAMPLE) += examples/

obj-$(CONFIG_GENERAL_OTA) += services/