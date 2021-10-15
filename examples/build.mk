ifdef CONFIG_ACCESS_ENABLED
obj-$(CONFIG_TM) += things_model/
obj-$(CONFIG_IOT_NBIOT) += onenet_nbiot/
obj-$(CONFIG_IOT_EDP) += onenet_edp/
obj-$(CONFIG_IOT_MQTT) += onenet_mqtt/
obj-$(CONFIG_IOT_MQTTS) += onenet_mqtts/
obj-$(CONFIG_RINGA) += ringa/
else
obj-$(CONFIG_GENERAL_OTA) += ota/
endif
