#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <cuda.h>
#include <cupti.h>

#include <zipkin/zipkin.hpp>

#include "util_cuda.hpp"
#include "util_cupti.hpp"
#include "tracer.hpp"

// FIXME - this should be per-thread most likely
typedef struct {
  dim3 gridDim;
  dim3 blockDim;
  size_t sharedMem;
  cudaStream_t stream;
  std::vector<uintptr_t> args;
  bool valid = false;
} ConfiguredCall_t;

ConfiguredCall_t &ConfiguredCall() {
  static ConfiguredCall_t cc;
  return cc;
}

static void handleCudaConfigureCall(const CUpti_CallbackData *cbInfo) {
  if (cbInfo->callbackSite == CUPTI_API_ENTER) {
    printf("callback: cudaConfigureCall entry\n");

    assert(!ConfiguredCall().valid && "call is already configured?\n");

    auto params = ((cudaConfigureCall_v3020_params *)(cbInfo->functionParams));
    ConfiguredCall().gridDim = params->gridDim;
    ConfiguredCall().blockDim = params->blockDim;
    ConfiguredCall().sharedMem = params->sharedMem;
    ConfiguredCall().stream = params->stream;
    ConfiguredCall().valid = true;
  } else if (cbInfo->callbackSite == CUPTI_API_EXIT) {
  } else {
    assert(0 && "How did we get here?");
  }
}

static void handleCudaSetupArgument(const CUpti_CallbackData *cbInfo) {
  if (cbInfo->callbackSite == CUPTI_API_ENTER) {
    printf("callback: cudaSetupArgument entry\n");
    const auto params =
        ((cudaSetupArgument_v3020_params *)(cbInfo->functionParams));
    const uintptr_t arg =
        (uintptr_t) * static_cast<const void *const *>(
                          params->arg); // arg is a pointer to the arg.
    // const size_t size     = params->size;
    // const size_t offset   = params->offset;

    assert(ConfiguredCall().valid);
    ConfiguredCall().args.push_back(arg);
  } else if (cbInfo->callbackSite == CUPTI_API_EXIT) {
  } else {
    assert(0 && "How did we get here?");
  }
}

static void handleCudaLaunch(const CUpti_CallbackData *cbInfo) {
  printf("callback: cudaLaunch preamble\n");
  const char *symbolName = cbInfo->symbolName;
  printf("launching %s\n", symbolName);


  // Get the current stream
  const cudaStream_t stream = ConfiguredCall().stream;
  (void) stream;

  for (size_t argIdx = 0; argIdx < ConfiguredCall().args.size();
       ++argIdx) {
      printf("kernel arg=%lu\n", ConfiguredCall().args[argIdx]);
  }


  if (cbInfo->callbackSite == CUPTI_API_ENTER) {

  } else if (cbInfo->callbackSite == CUPTI_API_EXIT) {
    printf("callback: cudaLaunch exit\n");
    ConfiguredCall().valid = false;
    ConfiguredCall().args.clear();
  } else {
    assert(0 && "How did we get here?");
  }

  printf("callback: cudaLaunch: done\n");
}

static void handleCudaMemcpy( const CUpti_CallbackData *cbInfo) {
  // extract API call parameters
  auto params = ((cudaMemcpy_v3020_params *)(cbInfo->functionParams));
  const uintptr_t dst = (uintptr_t)params->dst;
  const uintptr_t src = (uintptr_t)params->src;
  const cudaMemcpyKind kind = params->kind;
  const size_t count = params->count;
  (void) dst;
  (void) src;
  (void) kind;
  (void) count;


  if (cbInfo->callbackSite == CUPTI_API_ENTER) {
    printf("callback: cudaMemcpy entry\n");
    uint64_t start;
    CUPTI_CHECK(cuptiDeviceGetTimestamp(cbInfo->context, &start));
    printf("start: %lu\n", start);
  } else if (cbInfo->callbackSite == CUPTI_API_EXIT) {
    printf("callback: cudaMemcpy exit\n");
    uint64_t end;
    CUPTI_CHECK(cuptiDeviceGetTimestamp(cbInfo->context, &end));
    printf("end: %lu\n", end);

  } else {
    assert(0 && "How did we get here?");
  }
}

std::vector<zipkin::Span*> spanStack_;

zipkin::Span *stack_back() {
  if (spanStack_.empty()) {
    return Tracer::instance().global();
  } else {
    return spanStack_.back();
  }
}

void CUPTIAPI callback(void *userdata, CUpti_CallbackDomain domain,
                       CUpti_CallbackId cbid,
                       const CUpti_CallbackData *cbInfo) {
  (void)userdata;

  if (cbInfo->callbackSite == CUPTI_API_ENTER) {
    zipkin::Span *span  = stack_back()->span(""); // This name seems to do nothing
    span->with_name(cbInfo->functionName);
    spanStack_.push_back(span);
  }
  // zipkin::Span::Scope scope(*span);

  // Data is collected for the following APIs
  switch (domain) {
  case CUPTI_CB_DOMAIN_RUNTIME_API: {
    switch (cbid) {
    case CUPTI_RUNTIME_TRACE_CBID_cudaMemcpy_v3020:
      handleCudaMemcpy(cbInfo);
      break;
    case CUPTI_RUNTIME_TRACE_CBID_cudaLaunch_v3020:
      handleCudaLaunch(cbInfo);
      break;
    case CUPTI_RUNTIME_TRACE_CBID_cudaConfigureCall_v3020:
      handleCudaConfigureCall(cbInfo);
      break;
    case CUPTI_RUNTIME_TRACE_CBID_cudaSetupArgument_v3020:
      handleCudaSetupArgument(cbInfo);
      break;
    default:
      printf("skipping runtime call %s...\n", cbInfo->functionName);
      break;
    }
  } break;
  case CUPTI_CB_DOMAIN_DRIVER_API: {
    switch (cbid) {
    default:
      printf("skipping driver call %s...\n", cbInfo->functionName);
      break;
    }
  }
  default:
    break;
  }

  if (cbInfo->callbackSite == CUPTI_API_EXIT) {
    zipkin::Span *span  = stack_back();
  auto endpoint = Tracer::instance().endpoint();
  span->client_send(&endpoint);
  span->submit();
  spanStack_.pop_back();
  }
  
}

static int activateCallbacks() {

  static CUpti_SubscriberHandle SUBSCRIBER;

  CUPTI_CHECK(cuptiSubscribe(&SUBSCRIBER, (CUpti_CallbackFunc)callback, nullptr));
  CUPTI_CHECK(cuptiEnableDomain(1, SUBSCRIBER, CUPTI_CB_DOMAIN_RUNTIME_API));
  CUPTI_CHECK(cuptiEnableDomain(1, SUBSCRIBER, CUPTI_CB_DOMAIN_DRIVER_API));

return 0;
}

void onceSetupZipkin() {
  static bool done = false;
  if (!done) {
    done = true;
  }
}

static int activateZipkin() {

  for (int ii = 0; ii < 2; ii++) {
    // trace_name is the name of the trace
    zipkin::Span &span = *Tracer::instance().span("trace_name");
    zipkin::Span::Scope scope(span);

    auto endpoint = Tracer::instance().endpoint();
    span.client_send(&endpoint);

    span << std::make_pair("some_tag", "0.3." + std::to_string(ii));
    span << std::make_pair("another_tag", std::to_string(ii));
    span << std::make_pair("something_else", "something_else");
  }


  return 0;
}

// start callbacks only the first time
void onceActivateCallbacks() {
  static bool done = false;
  if (!done) {
    printf("Activating callbacks\n");
    activateCallbacks();
    done = true;
  }
}

// static int stopCallbacks() {
//   CUptiResult cuptierr;
//   cuptierr = cuptiUnsubscribe(SUBSCRIBER);
//   CUPTI_CHECK(cuptierr, "cuptiUnsubscribe");
//   return 0;
// }
