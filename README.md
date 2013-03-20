nnodes
======

Nnodes is an Authoritative-only DNS server, offers balancing, load-balancing, geographic balancing, redirection, and service-state-conscious failover with a focus on high performance, low latency service. 

zones/options file:

	see docs/zones.xml

zone files:

	see docs/local.xml


start service:

```bash
nnodes --cf <config file> [--df] [--ct]
```
