#include <Windows.h>
#include <node.h>

using namespace v8;

typedef struct {
  unsigned int version;
  int currentDV;
  int minDV;
  int maxDV;
} NV_DISPLAY_DVC_INFO;

typedef int (*nvapi_Initialize_t)();
typedef int *(*nvapi_QueryInterface_t)(unsigned int offset);
typedef int (*nvapi_GetDVCInfo_t)(int displayHandle, int outputId,
                                  NV_DISPLAY_DVC_INFO *DVCInfo);
typedef int (*nvapi_EnumNvidiaDisplayHandle_t)(int thisEnum, int *handle);
typedef int (*nvapi_EnumPhysicalGPUs_t)(int **handles, int *count);
typedef int (*nvapi_SetDVCLevel_t)(int handle, int outputId, int level);

nvapi_QueryInterface_t nvapi_QueryInterface = NULL;
nvapi_Initialize_t nvapi_Initialize = NULL;
nvapi_GetDVCInfo_t nvapi_GetDVCInfo = NULL;
nvapi_EnumNvidiaDisplayHandle_t nvapi_EnumNvidiaDisplayHandle = NULL;
nvapi_EnumPhysicalGPUs_t nvapi_EnumPhysicalGPUs = NULL;
nvapi_SetDVCLevel_t nvapi_SetDVCLevel = NULL;

int handle = 0;

int InitializeNVApi() {
  if (handle > 0) {
    return handle;
  }

  Isolate *isolate = Isolate::GetCurrent();

  HMODULE nvapi_h = LoadLibrary("nvapi64.dll");
  if (nvapi_h == NULL) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Could not load nvapi64.dll")
            .ToLocalChecked()));
    return -1;
  }

  nvapi_QueryInterface =
      (nvapi_QueryInterface_t)GetProcAddress(nvapi_h, "nvapi_QueryInterface");
  if (nvapi_QueryInterface == NULL) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate,
                            "Could not find function NvAPI_QueryInterface")
            .ToLocalChecked()));
    return -1;
  }

  nvapi_Initialize = (nvapi_Initialize_t)(*nvapi_QueryInterface)(0x0150E828);
  int initialized = (*nvapi_Initialize)();
  if (initialized != 0) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Could not initialize nvapi")
            .ToLocalChecked()));
    return initialized;
  }

  nvapi_EnumNvidiaDisplayHandle =
      (nvapi_EnumNvidiaDisplayHandle_t)(*nvapi_QueryInterface)(0x9ABDD40D);
  int status = (*nvapi_EnumNvidiaDisplayHandle)(0, &handle);
  if (status != 0) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Could not get display handle")
            .ToLocalChecked()));
    return status;
  }

  return 0;
}

NV_DISPLAY_DVC_INFO getDVCInfo() {
  Isolate *isolate = Isolate::GetCurrent();

  NV_DISPLAY_DVC_INFO info = {};
  info.version = sizeof(NV_DISPLAY_DVC_INFO) | 0x10000;

  nvapi_GetDVCInfo = (nvapi_GetDVCInfo_t)(*nvapi_QueryInterface)(0x4085DE45);
  if (nvapi_GetDVCInfo == NULL) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Could not find `nvapi_GetDVCInfo_t`")
            .ToLocalChecked()));
    return info;
  }

  int status = (*nvapi_GetDVCInfo)(handle, 0, &info);
  if (status != 0) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Could not get digital vibrance")
            .ToLocalChecked()));
    return info;
  }

  return info;
}

void GetDigitalVibrance(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();

  NV_DISPLAY_DVC_INFO info = getDVCInfo();

  Local<Context> context = isolate->GetCurrentContext();
  Local<Object> obj = Object::New(isolate);
  obj->Set(context, String::NewFromUtf8(isolate, "minDV").ToLocalChecked(),
           Number::New(isolate, info.minDV))
      .FromJust();
  obj->Set(context, String::NewFromUtf8(isolate, "maxDV").ToLocalChecked(),
           Number::New(isolate, info.maxDV))
      .FromJust();
  obj->Set(context, String::NewFromUtf8(isolate, "currentDV").ToLocalChecked(),
           Number::New(isolate, info.currentDV))
      .FromJust();

  args.GetReturnValue().Set(obj);
}

void ToggleDigitalVibrance(const FunctionCallbackInfo<Value> &args) {
  Isolate *isolate = args.GetIsolate();

  NV_DISPLAY_DVC_INFO info = getDVCInfo();

  nvapi_SetDVCLevel = (nvapi_SetDVCLevel_t)(*nvapi_QueryInterface)(0x172409B4);
  if (nvapi_SetDVCLevel == NULL) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Could not find `nvapi_SetDVCLevel`")
            .ToLocalChecked()));
    return;
  }

  int status = (*nvapi_SetDVCLevel)(
      handle, 0, info.currentDV == 0 ? info.maxDV : info.minDV);
  if (status != 0) {
    isolate->ThrowException(Exception::TypeError(
        String::NewFromUtf8(isolate, "Could not toggle digital vibrance")
            .ToLocalChecked()));
    return;
  }

  Local<Number> num = Number::New(isolate, status);

  args.GetReturnValue().Set(num);
}

void Init(Local<Object> exports) {
  if (InitializeNVApi() != 0) {
    return;
  }

  NODE_SET_METHOD(exports, "getDigitalVibrance", GetDigitalVibrance);
  NODE_SET_METHOD(exports, "toggleDigitalVibrance", ToggleDigitalVibrance);
}

NODE_MODULE(NODE_GYP_MODULE_NAME, Init)
