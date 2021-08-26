/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "AidlConversion"

#include <android_os_Parcel.h>
#include <binder/Parcel.h>
#include <jni.h>
#include <log/log.h>
#include <media/AidlConversion.h>
#include <system/audio.h>

#include "core_jni_helpers.h"

namespace {

using namespace android;

#define PACKAGE "android/media/audio/common"
#define CLASSNAME PACKAGE "/AidlConversion"

template <typename AidlType, typename ConvFunc>
int aidl2legacy(JNIEnv* env, jobject clazz, jobject jParcel, const ConvFunc& conv,
                int fallbackValue) {
    if (Parcel* parcel = parcelForJavaObject(env, jParcel); parcel != nullptr) {
        AidlType aidl{};
        if (status_t status = aidl.readFromParcel(parcel); status == OK) {
            auto legacy = conv(aidl);
            if (legacy.ok()) {
                return legacy.value();
            }
        } else {
            ALOGE("aidl2legacy: Failed to read from parcel: %d", status);
        }
    } else {
        ALOGE("aidl2legacy: Failed to retrieve the native parcel from Java parcel");
    }
    return fallbackValue;
}

template <typename LegacyType, typename ConvFunc>
jobject legacy2aidl(JNIEnv* env, jobject clazz, LegacyType legacy, const ConvFunc& conv) {
    auto aidl = conv(legacy);
    if (!aidl.ok()) {
        return 0;
    }
    if (jobject jParcel = createJavaParcelObject(env); jParcel != 0) {
        if (Parcel* parcel = parcelForJavaObject(env, jParcel); parcel != nullptr) {
            if (status_t status = aidl.value().writeToParcel(parcel); status == OK) {
                parcel->setDataPosition(0);
                return jParcel;
            }
        } else {
            ALOGE("legacy2aidl: Failed to retrieve the native parcel from Java parcel");
        }
        env->DeleteLocalRef(jParcel);
    } else {
        ALOGE("legacy2aidl: Failed to create Java parcel");
    }
    return 0;
}

int aidl2legacy_AudioChannelLayout_Parcel_audio_channel_mask_t(JNIEnv* env, jobject clazz,
                                                               jobject jParcel, jboolean isInput) {
    return aidl2legacy<media::audio::common::AudioChannelLayout>(
            env, clazz, jParcel,
            [isInput](const media::audio::common::AudioChannelLayout& l) {
                return aidl2legacy_AudioChannelLayout_audio_channel_mask_t(l, isInput);
            },
            AUDIO_CHANNEL_INVALID);
}

jobject legacy2aidl_audio_channel_mask_t_AudioChannelLayout_Parcel(
        JNIEnv* env, jobject clazz, int /*audio_channel_mask_t*/ legacy, jboolean isInput) {
    return legacy2aidl<audio_channel_mask_t>(
            env, clazz, static_cast<audio_channel_mask_t>(legacy),
            [isInput](audio_channel_mask_t m) {
                return legacy2aidl_audio_channel_mask_t_AudioChannelLayout(m, isInput);
            });
}

int aidl2legacy_AudioFormatDescription_Parcel_audio_format_t(JNIEnv* env, jobject clazz,
                                                             jobject jParcel) {
    return aidl2legacy<
            media::audio::common::
                    AudioFormatDescription>(env, clazz, jParcel,
                                            aidl2legacy_AudioFormatDescription_audio_format_t,
                                            AUDIO_FORMAT_INVALID);
}

jobject legacy2aidl_audio_format_t_AudioFormatDescription_Parcel(JNIEnv* env, jobject clazz,
                                                                 int /*audio_format_t*/ legacy) {
    return legacy2aidl<audio_format_t>(env, clazz, static_cast<audio_format_t>(legacy),
                                       legacy2aidl_audio_format_t_AudioFormatDescription);
}

const JNINativeMethod gMethods[] = {
        {"aidl2legacy_AudioChannelLayout_Parcel_audio_channel_mask_t", "(Landroid/os/Parcel;Z)I",
         reinterpret_cast<void*>(aidl2legacy_AudioChannelLayout_Parcel_audio_channel_mask_t)},
        {"legacy2aidl_audio_channel_mask_t_AudioChannelLayout_Parcel", "(IZ)Landroid/os/Parcel;",
         reinterpret_cast<void*>(legacy2aidl_audio_channel_mask_t_AudioChannelLayout_Parcel)},
        {"aidl2legacy_AudioFormatDescription_Parcel_audio_format_t", "(Landroid/os/Parcel;)I",
         reinterpret_cast<void*>(aidl2legacy_AudioFormatDescription_Parcel_audio_format_t)},
        {"legacy2aidl_audio_format_t_AudioFormatDescription_Parcel", "(I)Landroid/os/Parcel;",
         reinterpret_cast<void*>(legacy2aidl_audio_format_t_AudioFormatDescription_Parcel)},
};

} // namespace

int register_android_media_audio_common_AidlConversion(JNIEnv* env) {
    return RegisterMethodsOrDie(env, CLASSNAME, gMethods, NELEM(gMethods));
}
