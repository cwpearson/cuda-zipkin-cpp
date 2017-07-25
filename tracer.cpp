#include "tracer.hpp"

Tracer &Tracer::instance() {
  const auto url = "http://127.0.0.1:9411/api/v1/spans";
  const auto endpoint_addr = "127.0.0.1";
  const auto port = 9411;

  static Tracer t_(url, endpoint_addr, port);
  return t_;
}