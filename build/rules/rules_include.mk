inc-y := 
inc-y += platforms/include
inc-y += common

inc-$(CONFIG_NETWORK_TLS) += security/tls

inc-$(CONFIG_CJSON) += tools/cJSON
inc-$(CONFIG_MOD_ADAPTOR) += tools/modadptr
inc-$(CONFIG_MOD_ADAPTOR_M5311) += tools/modadptr/m5311
inc-$(CONFIG_WOLFSSL_TLS) += tools/wolfssl/port tools/wolfssl/wolfssl-3.15.3
inc-$(CONFIG_WOLFSSL_CRYPTO) += tools/wolfssl/port tools/wolfssl/wolfssl-3.15.3

inc-$(CONFIG_MQTT) += protocols/mqtt
inc-$(CONFIG_HTTP) += protocols/http
inc-$(CONFIG_COAP) += protocols/coap/er-coap-13

inc-$(CONFIG_ACCESS_TOKEN) += onenet/utils
inc-$(CONFIG_ONENET_PAYLOAD) += onenet/utils
inc-$(CONFIG_RINGA) += onenet/ringa
inc-$(CONFIG_TM) += onenet/tm
inc-$(CONFIG_IOT) += onenet/iot
inc-$(CONFIG_IOT_EDP) += onenet/iot/edp
inc-$(CONFIG_IOT_MQTT) += onenet/iot/mqtt
inc-$(CONFIG_IOT_MQTTS) += onenet/iot/mqtts
inc-$(CONFIG_IOT_NBIOT) += onenet/iot/nbiot

inc-$(CONFIG_GENERAL_OTA) += services/ota
