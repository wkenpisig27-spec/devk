#pragma once

// Network audit diagnostics for Option B / Phase-6 refactoring.
// Default OFF — enable in project preprocessor or before including headers:
//   #define NET_AUDIT_DIAG 1
//
// When enabled, logs one line per received packet in Receiver::Process and
// optional dispatch tracing in application OnProcessData hooks.

#ifndef NET_AUDIT_DIAG
#define NET_AUDIT_DIAG 0
#endif

#if NET_AUDIT_DIAG
#define NET_AUDIT_LOG(tag, fmt, ...) LG(tag, fmt, ##__VA_ARGS__)
#else
#define NET_AUDIT_LOG(tag, fmt, ...) ((void)0)
#endif
