AUDIO_PATH := $(call my-dir)

ifeq ($(INTEL_VA),true)
 include $(AUDIO_PATH)/mixvbp/Android.mk
 include $(AUDIO_PATH)/videodecoder/Android.mk
 include $(AUDIO_PATH)/videoencoder/Android.mk
 include $(AUDIO_PATH)/imageencoder/Android.mk
 include $(AUDIO_PATH)/imagedecoder/Android.mk
endif
