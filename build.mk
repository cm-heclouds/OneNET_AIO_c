obj-y += platforms/
obj-y += common/

obj-$(CONFIG_ACCESS_ENABLED) += onenet/
obj-$(CONFIG_ACCESS_TOKEN) += onenet/

obj-$(CONFIG_MQTT) += protocols/mqtt/
obj-$(CONFIG_HTTP) += protocols/http/
obj-$(CONFIG_COAP) += protocols/coap/

obj-$(CONFIG_NETWORK_TLS) += security/tls/

obj-$(CONFIG_CJSON) += tools/
obj-$(CONFIG_WOLFSSL_TLS) += tools/
obj-$(CONFIG_WOLFSSL_CRYPTO) += tools/
obj-$(CONFIG_MOD_ADAPTOR) += tools/

obj-$(CONFIG_GENERAL_OTA) += services/ota/

obj-$(CONFIG_BUILD_EXAMPLE) += examples/
