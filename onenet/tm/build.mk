obj-$(CONFIG_TM_MQTT) += tm_mqtt.o
obj-$(CONFIG_TM_COAP) += tm_coap.o
obj-y += tm_data.o
obj-y += tm_api.o
obj-y += tm_onejson.o
obj-$(CONFIG_TM_GATEWAY) += tm_subdev.o