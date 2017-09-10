#include "route.h"

/* allow only one route, the default one */
route_t dft_route;

#ifdef CONFIG_IPV6
route6_t dft_route6;
#endif
