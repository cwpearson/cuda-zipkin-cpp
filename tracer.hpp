#ifndef TRACER_HPP
#define TRACER_HPP

#include <memory>
#include <string>

#include <zipkin/zipkin.hpp>

class Tracer
{
  private:
    sockaddr_in endpoint_addr_;

    std::unique_ptr<zipkin::Tracer> tracer_;
    std::unique_ptr<zipkin::HttpCollector> collector_;

    zipkin::Span *global_span_;

  public:
    Tracer(const char *url, const char *endpoint_addr, const int port) : tracer_(nullptr), collector_(nullptr), global_span_(nullptr)
    {

        std::unique_ptr<zipkin::HttpConf> conf(new zipkin::HttpConf(url));
        collector_ = std::unique_ptr<zipkin::HttpCollector>(conf->create());
        tracer_ = std::unique_ptr<zipkin::Tracer>(zipkin::Tracer::create(collector_.get()));

        if (collector_.get())
        {
            conf.release();
        }
        endpoint_addr_.sin_addr.s_addr = inet_addr(endpoint_addr);
        endpoint_addr_.sin_port = htons(port);

        global_span_ = tracer_->span("global");
    }

    ~Tracer()
    {
        fprintf(stderr, "tracer dtor\n");
        auto endpoint = Tracer::instance().endpoint();
        global_span_->client_send(&endpoint);
        global_span_->submit();

        fprintf(stderr, "Shutting down collector...\n");
        collector()->shutdown(std::chrono::seconds(5));

        fprintf(stderr, "tracer dtor done!\n");
    }

    zipkin::Span *span(const char *name)
    {   return global_span_->span(name);
    }
    
    zipkin::Endpoint endpoint()
    {
        return zipkin::Endpoint("service_name", &endpoint_addr_);
    }

    std::unique_ptr<zipkin::HttpCollector> &collector()
    {
        return collector_;
    }

    static Tracer &instance();
};

#endif