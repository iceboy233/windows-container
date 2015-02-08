#include "core/util.h"

#include <cstdlib>

namespace winc {

ProcThreadAttributeList::~ProcThreadAttributeList() {
  if (data_) {
    DeleteProcThreadAttributeList(data_);
    free(data_);
  }
}

ResultCode ProcThreadAttributeList::Init(
    DWORD attribute_count, DWORD flags) {
  SIZE_T size;
  if (!::InitializeProcThreadAttributeList(NULL, attribute_count,
                                           flags, &size) &&
      ::GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    return WINC_ERROR_UTIL;
  LPPROC_THREAD_ATTRIBUTE_LIST data =
    reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(malloc(size));
  if (!::InitializeProcThreadAttributeList(data, attribute_count,
                                           flags, &size)) {
    free(data);
    return WINC_ERROR_UTIL;
  }
  data_ = data;
  return WINC_OK;
}

ResultCode ProcThreadAttributeList::Update(
    DWORD flags, DWORD_PTR attribute, PVOID value, SIZE_T size) {
  if (!::UpdateProcThreadAttribute(data_,
      flags, attribute, value, size, NULL, NULL))
    return WINC_ERROR_UTIL;
  return WINC_OK;
}

}
