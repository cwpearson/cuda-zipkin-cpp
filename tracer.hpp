#ifndef TRACER_HPP
#define TRACER_HPP

#include <memory>
#include <string>

#include <zipkin/zipkin.hpp>

class Tracer {
private:
  sockaddr_in endpoint_addr_;

  std::unique_ptr<zipkin::Tracer> tracer_;
  std::unique_ptr<zipkin::HttpCollector> collector_;

public:
    Tracer(const char *url, const char *endpoint_addr, const int port) : tracer_(nullptr), collector_(nullptr) {

          std::unique_ptr<zipkin::HttpConf> conf(new zipkin::HttpConf(url));
  collector_ = std::unique_ptr<zipkin::HttpCollector>(conf->create());
  tracer_ = std::unique_ptr<zipkin::Tracer>(  zipkin::Tracer::create(collector_.get()) );

  if (collector_.get()) {
    conf.release();
  }
  endpoint_addr_.sin_addr.s_addr = inet_addr(endpoint_addr);
  endpoint_addr_.sin_port = htons(port);
    }

    zipkin::Span* span(const char *name) {
        return tracer_->span(name);
    }
    zipkin::Endpoint endpoint(const char *name) {
        return zipkin::Endpoint(name, &endpoint_addr_);
    }

    std::unique_ptr<zipkin::HttpCollector> &collector() {
        return collector_;
    }

static Tracer &instance();
};

#endif