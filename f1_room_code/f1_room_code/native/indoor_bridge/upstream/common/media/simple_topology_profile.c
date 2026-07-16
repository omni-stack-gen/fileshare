/**
 * @file
 * @brief simple 示例应用公共模块 simple_topology_profile 的实现文件。
 *
 * 本文件属于 examples 应用层，用于板端/host simple 示例验证，不作为 lib/mtrans 核心库接口。
 */

#include "simple_topology_profile.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <utils/util.h>

static int dta_copy(char *dst, size_t dst_len, const char *src)
{
	size_t src_len;

	if (!dst || dst_len == 0 || !IS_VALID_STRING(src))
	{
		return -EINVAL;
	}

	src_len = safe_strncpy(dst, src, dst_len);
	return src_len < dst_len ? 0 : -ENOSPC;
}


static void dta_log(const simple_topology_profile_prepare_audio_t *cfg, const char *line)
{
	if (cfg && cfg->log_cb && line)
	{
		cfg->log_cb(cfg->log_user_ctx, line);
	}
}

static void dta_log_fail(const simple_topology_profile_prepare_audio_t *cfg, int ret,
                         const char *reason, const char *local, const char *peer)
{
	char line[512];

	snprintf(line, sizeof(line),
	         "topology audio endpoint failed: node=%s ret=%d "
	         "reason=%s local=%s peer=%s session=0x%llx",
	         cfg && cfg->node_name ? cfg->node_name : "-",
	         ret,
	         reason ? reason : "-",
	         local ? local : "-",
	         peer ? peer : "-",
	         (unsigned long long)(cfg ? cfg->session_id : 0));
	dta_log(cfg, line);
}

static int dta_add_node(topology_ctx_t *ctx, const char *device_id, const char *profile)
{
	topology_node_t node;

	memset(&node, 0, sizeof(node));
	if (dta_copy(node.device_id, sizeof(node.device_id), device_id) < 0)
	{
		return -EINVAL;
	}
	node.device_type = TOPOLOGY_DEVICE_UNKNOWN;
	(void)dta_copy(node.topology_profile, sizeof(node.topology_profile), profile);
	node.binding_state = 1;
	node.presence_state = 1;
	return topology_upsert_node(ctx, &node);
}

static int dta_add_domain(topology_ctx_t *ctx, const char *domain_id,
                          topology_domain_kind_t domain_kind,
                          const char *owner_device_id, const char *profile)
{
	topology_domain_t domain;

	memset(&domain, 0, sizeof(domain));
	if (dta_copy(domain.domain_id, sizeof(domain.domain_id), domain_id) < 0)
	{
		return -EINVAL;
	}
	domain.domain_kind = domain_kind;
	(void)dta_copy(domain.owner_device_id, sizeof(domain.owner_device_id), owner_device_id);
	(void)dta_copy(domain.topology_profile, sizeof(domain.topology_profile), profile);
	return topology_upsert_domain(ctx, &domain);
}

static int dta_add_role(topology_ctx_t *ctx, const char *device_id,
                        const char *domain_id, topology_role_kind_t role_kind)
{
	topology_role_t role;

	memset(&role, 0, sizeof(role));
	if (dta_copy(role.device_id, sizeof(role.device_id), device_id) < 0 ||
	    dta_copy(role.domain_id, sizeof(role.domain_id), domain_id) < 0)
	{
		return -EINVAL;
	}
	role.role_kind = role_kind;
	return topology_upsert_role(ctx, &role);
}

static int dta_add_link(topology_ctx_t *ctx, const char *src_device_id,
                        const char *dst_device_id, const char *domain_id,
                        topology_link_kind_t link_kind, uint32_t capabilities)
{
	topology_link_t link;

	memset(&link, 0, sizeof(link));
	if (dta_copy(link.src_device_id, sizeof(link.src_device_id), src_device_id) < 0 ||
	    dta_copy(link.dst_device_id, sizeof(link.dst_device_id), dst_device_id) < 0 ||
	    dta_copy(link.domain_id, sizeof(link.domain_id), domain_id) < 0)
	{
		return -EINVAL;
	}
	link.link_kind = link_kind;
	link.capabilities = capabilities;
	return topology_upsert_link(ctx, &link);
}

static int dta_add_endpoint(topology_ctx_t *ctx, const char *device_id,
                            const char *domain_id, topology_link_kind_t link_kind,
                            uint32_t addr, uint16_t port)
{
	topology_endpoint_t endpoint;

	memset(&endpoint, 0, sizeof(endpoint));
	if (dta_copy(endpoint.device_id, sizeof(endpoint.device_id), device_id) < 0 ||
	    dta_copy(endpoint.domain_id, sizeof(endpoint.domain_id), domain_id) < 0)
	{
		return -EINVAL;
	}
	endpoint.link_kind = link_kind;
	endpoint.addr = addr;
	endpoint.port = port;
	return topology_upsert_endpoint(ctx, &endpoint);
}

static bool dta_generic_profile_valid(const simple_topology_profile_generic_t *profile)
{
	if (!profile || profile->node_count == 0 || !profile->nodes)
	{
		return false;
	}
	if ((profile->domain_count > 0 && !profile->domains) ||
	    (profile->role_count > 0 && !profile->roles) ||
	    (profile->link_count > 0 && !profile->links) ||
	    (profile->endpoint_count > 0 && !profile->endpoints))
	{
		return false;
	}
	return true;
}

static bool dta_topology_has_node(const topology_ctx_t *topology, const char *device_id)
{
	topology_summary_t summary;
	topology_node_t node;
	size_t i;

	if (!IS_VALID_STRING(device_id) || topology_get_summary(topology, &summary) != 0)
	{
		return false;
	}
	for (i = 0; i < summary.node_count; ++i)
	{
		if (topology_get_node_at(topology, i, &node) == 0 &&
		    strcmp(node.device_id, device_id) == 0)
		{
			return true;
		}
	}
	return false;
}

static bool dta_topology_has_domain(const topology_ctx_t *topology, const char *domain_id)
{
	topology_summary_t summary;
	topology_domain_t domain;
	size_t i;

	if (!IS_VALID_STRING(domain_id) || topology_get_summary(topology, &summary) != 0)
	{
		return false;
	}
	for (i = 0; i < summary.domain_count; ++i)
	{
		if (topology_get_domain_at(topology, i, &domain) == 0 &&
		    strcmp(domain.domain_id, domain_id) == 0)
		{
			return true;
		}
	}
	return false;
}

static bool dta_topology_has_role(const topology_ctx_t *topology, const char *device_id,
                                  const char *domain_id, topology_role_kind_t role_kind)
{
	topology_summary_t summary;
	topology_role_t role;
	size_t i;

	if (!IS_VALID_STRING(device_id) || !IS_VALID_STRING(domain_id) ||
	    topology_get_summary(topology, &summary) != 0)
	{
		return false;
	}
	for (i = 0; i < summary.role_count; ++i)
	{
		if (topology_get_role_at(topology, i, &role) == 0 &&
		    strcmp(role.device_id, device_id) == 0 &&
		    strcmp(role.domain_id, domain_id) == 0 &&
		    role.role_kind == role_kind)
		{
			return true;
		}
	}
	return false;
}

static bool dta_topology_has_plc_link(const topology_ctx_t *topology, const char *src,
                                      const char *dst, const char *domain_id)
{
	topology_summary_t summary;
	topology_link_t link;
	size_t i;

	if (!IS_VALID_STRING(src) || !IS_VALID_STRING(dst) || !IS_VALID_STRING(domain_id) ||
	    topology_get_summary(topology, &summary) != 0)
	{
		return false;
	}
	for (i = 0; i < summary.link_count; ++i)
	{
		bool same_direction;
		bool reverse_direction;

		if (topology_get_link_at(topology, i, &link) != 0 ||
		    strcmp(link.domain_id, domain_id) != 0 ||
		    link.link_kind != TOPOLOGY_LINK_PLC)
		{
			continue;
		}
		same_direction = strcmp(link.src_device_id, src) == 0 &&
		                 strcmp(link.dst_device_id, dst) == 0;
		reverse_direction = strcmp(link.src_device_id, dst) == 0 &&
		                    strcmp(link.dst_device_id, src) == 0;
		if (same_direction || reverse_direction)
		{
			return true;
		}
	}
	return false;
}

static bool dta_topology_has_endpoint(const topology_ctx_t *topology, const char *device_id,
                                      const char *domain_id, topology_link_kind_t link_kind)
{
	topology_summary_t summary;
	topology_endpoint_t endpoint;
	size_t i;

	if (!IS_VALID_STRING(device_id) || !IS_VALID_STRING(domain_id) ||
	    topology_get_summary(topology, &summary) != 0)
	{
		return false;
	}
	for (i = 0; i < summary.endpoint_count; ++i)
	{
		if (topology_get_endpoint_at(topology, i, &endpoint) == 0 &&
		    strcmp(endpoint.device_id, device_id) == 0 &&
		    strcmp(endpoint.domain_id, domain_id) == 0 &&
		    endpoint.link_kind == link_kind)
		{
			return true;
		}
	}
	return false;
}

static int dta_plc_cco_preflight(
	const topology_ctx_t *topology,
	const simple_topology_profile_plc_cco_t *cfg)
{
	topology_summary_t summary;
	size_t add_nodes = 0;
	size_t add_domains = 0;
	size_t add_roles = 0;
	size_t add_links = 0;
	size_t add_endpoints = 0;
	size_t i;
	int ret;

	ret = topology_get_summary(topology, &summary);
	if (ret != 0)
	{
		return ret;
	}

	for (i = 0; i < cfg->sta_count; ++i)
	{
		size_t j;

		if (!IS_VALID_STRING(cfg->stas[i].device_id) ||
		    cfg->stas[i].plc_addr == 0)
		{
			return -EINVAL;
		}
		for (j = i + 1; j < cfg->sta_count; ++j)
		{
			if (IS_VALID_STRING(cfg->stas[j].device_id) &&
			    strcmp(cfg->stas[i].device_id, cfg->stas[j].device_id) == 0)
			{
				return -EINVAL;
			}
		}
	}

	add_nodes += dta_topology_has_node(topology, cfg->cco_device_id) ? 0U : 1U;
	add_domains += dta_topology_has_domain(topology, cfg->domain_id) ? 0U : 1U;
	add_roles += dta_topology_has_role(topology, cfg->cco_device_id, cfg->domain_id,
	                                   TOPOLOGY_ROLE_PLC_COORDINATOR) ? 0U : 1U;
	add_endpoints += dta_topology_has_endpoint(topology, cfg->cco_device_id,
	                                           cfg->domain_id, TOPOLOGY_LINK_PLC) ? 0U : 1U;

	for (i = 0; i < cfg->sta_count; ++i)
	{
		const simple_topology_profile_plc_sta_t *sta = &cfg->stas[i];

		add_nodes += dta_topology_has_node(topology, sta->device_id) ? 0U : 1U;
		add_roles += dta_topology_has_role(topology, sta->device_id, cfg->domain_id,
		                                   TOPOLOGY_ROLE_PLC_STATION) ? 0U : 1U;
		add_links += dta_topology_has_plc_link(topology, cfg->cco_device_id,
		                                       sta->device_id, cfg->domain_id) ? 0U : 1U;
		add_endpoints += dta_topology_has_endpoint(topology, sta->device_id,
		                                           cfg->domain_id, TOPOLOGY_LINK_PLC) ? 0U : 1U;
	}

	if (summary.node_count + add_nodes > TOPOLOGY_MAX_NODES ||
	    summary.domain_count + add_domains > TOPOLOGY_MAX_DOMAINS ||
	    summary.role_count + add_roles > TOPOLOGY_MAX_ROLES ||
	    summary.link_count + add_links > TOPOLOGY_MAX_LINKS ||
	    summary.endpoint_count + add_endpoints > TOPOLOGY_MAX_ENDPOINTS)
	{
		return -ENOSPC;
	}
	return 0;
}

topology_ctx_t *simple_topology_profile_create_generic(
	const simple_topology_profile_generic_t *profile)
{
	topology_ctx_t *ctx;
	size_t i;

	if (!dta_generic_profile_valid(profile))
	{
		return NULL;
	}

	ctx = topology_create(NULL);
	if (!ctx)
	{
		return NULL;
	}

	for (i = 0; i < profile->node_count; ++i)
	{
		if (topology_upsert_node(ctx, &profile->nodes[i]) != 0)
		{
			topology_destroy(ctx);
			return NULL;
		}
	}
	for (i = 0; i < profile->domain_count; ++i)
	{
		if (topology_upsert_domain(ctx, &profile->domains[i]) != 0)
		{
			topology_destroy(ctx);
			return NULL;
		}
	}
	for (i = 0; i < profile->role_count; ++i)
	{
		if (topology_upsert_role(ctx, &profile->roles[i]) != 0)
		{
			topology_destroy(ctx);
			return NULL;
		}
	}
	for (i = 0; i < profile->link_count; ++i)
	{
		if (topology_upsert_link(ctx, &profile->links[i]) != 0)
		{
			topology_destroy(ctx);
			return NULL;
		}
	}
	for (i = 0; i < profile->endpoint_count; ++i)
	{
		if (topology_upsert_endpoint(ctx, &profile->endpoints[i]) != 0)
		{
			topology_destroy(ctx);
			return NULL;
		}
	}
	return ctx;
}

int simple_topology_profile_add_plc_cco(
	topology_ctx_t *topology,
	const simple_topology_profile_plc_cco_t *cfg)
{
	const char *profile;
	uint32_t capabilities;
	size_t i;
	int ret;

	if (!topology || !cfg || !IS_VALID_STRING(cfg->cco_device_id) ||
	    !IS_VALID_STRING(cfg->domain_id) ||
	    cfg->domain_kind == TOPOLOGY_DOMAIN_UNKNOWN ||
	    cfg->cco_plc_addr == 0 || cfg->plc_port == 0 ||
	    !cfg->stas || cfg->sta_count == 0)
	{
		return -EINVAL;
	}

	ret = dta_plc_cco_preflight(topology, cfg);
	if (ret != 0)
	{
		return ret;
	}

	profile = IS_VALID_STRING(cfg->profile_name) ? cfg->profile_name : "generic_plc";
	capabilities = cfg->capabilities != 0 ?
	               cfg->capabilities : (TOPOLOGY_CAP_CONTROL | TOPOLOGY_CAP_AUDIO);

	ret = dta_add_node(topology, cfg->cco_device_id, profile);
	if (ret == 0)
	{
		ret = dta_add_domain(topology, cfg->domain_id, cfg->domain_kind,
		                     cfg->cco_device_id, profile);
	}
	if (ret == 0)
	{
		ret = dta_add_role(topology, cfg->cco_device_id, cfg->domain_id,
		                   TOPOLOGY_ROLE_PLC_COORDINATOR);
	}
	if (ret == 0)
	{
		ret = dta_add_endpoint(topology, cfg->cco_device_id, cfg->domain_id,
		                       TOPOLOGY_LINK_PLC, cfg->cco_plc_addr,
		                       cfg->plc_port);
	}
	if (ret != 0)
	{
		return ret;
	}

	for (i = 0; i < cfg->sta_count; ++i)
	{
		const simple_topology_profile_plc_sta_t *sta = &cfg->stas[i];

		if (!IS_VALID_STRING(sta->device_id) || sta->plc_addr == 0)
		{
			return -EINVAL;
		}
		ret = dta_add_node(topology, sta->device_id, profile);
		if (ret == 0)
		{
			ret = dta_add_role(topology, sta->device_id, cfg->domain_id,
			                   TOPOLOGY_ROLE_PLC_STATION);
		}
		if (ret == 0)
		{
			ret = dta_add_link(topology, cfg->cco_device_id, sta->device_id,
			                   cfg->domain_id, TOPOLOGY_LINK_PLC, capabilities);
		}
		if (ret == 0)
		{
			ret = dta_add_endpoint(topology, sta->device_id, cfg->domain_id,
			                       TOPOLOGY_LINK_PLC, sta->plc_addr,
			                       cfg->plc_port);
		}
		if (ret != 0)
		{
			return ret;
		}
	}
	return 0;
}

int simple_topology_profile_add_converter_dual_domain(
	topology_ctx_t *topology,
	const simple_topology_profile_converter_dual_domain_t *cfg)
{
	static const char default_downlink_domain[] = "unit-downlink";
	static const char default_plc_domain[] = "converter-plc";
	const char *profile;
	const char *downlink_domain;
	const char *plc_domain;
	uint32_t capabilities;
	simple_topology_profile_plc_cco_t cco;
	int ret;

	if (!topology || !cfg ||
	    !IS_VALID_STRING(cfg->unit_device_id) ||
	    !IS_VALID_STRING(cfg->converter_device_id) ||
	    cfg->unit_downlink_addr == 0 ||
	    cfg->converter_downlink_addr == 0 ||
	    cfg->downlink_port == 0 ||
	    cfg->converter_plc_addr == 0 ||
	    cfg->plc_port == 0 ||
	    !cfg->stas ||
	    cfg->sta_count == 0)
	{
		return -EINVAL;
	}

	profile = IS_VALID_STRING(cfg->profile_name) ?
	          cfg->profile_name : "converter_dual_domain";
	downlink_domain = IS_VALID_STRING(cfg->downlink_domain_id) ?
	                  cfg->downlink_domain_id : default_downlink_domain;
	plc_domain = IS_VALID_STRING(cfg->plc_domain_id) ?
	             cfg->plc_domain_id : default_plc_domain;
	capabilities = cfg->capabilities != 0 ?
	               cfg->capabilities : (TOPOLOGY_CAP_CONTROL | TOPOLOGY_CAP_AUDIO);

	memset(&cco, 0, sizeof(cco));
	cco.profile_name = profile;
	cco.cco_device_id = cfg->converter_device_id;
	cco.domain_id = plc_domain;
	cco.domain_kind = TOPOLOGY_DOMAIN_CONVERTER_PLC;
	cco.cco_plc_addr = cfg->converter_plc_addr;
	cco.plc_port = cfg->plc_port;
	cco.capabilities = capabilities;
	cco.stas = cfg->stas;
	cco.sta_count = cfg->sta_count;
	ret = dta_plc_cco_preflight(topology, &cco);
	if (ret != 0)
	{
		return ret;
	}

	ret = dta_add_node(topology, cfg->unit_device_id, profile);
	if (ret == 0)
	{
		ret = dta_add_node(topology, cfg->converter_device_id, profile);
	}
	if (ret == 0)
	{
		ret = dta_add_domain(topology, downlink_domain,
		                     TOPOLOGY_DOMAIN_UNIT_DOWNLINK_SPE,
		                     cfg->unit_device_id, profile);
	}
	if (ret == 0)
	{
		ret = dta_add_role(topology, cfg->unit_device_id, downlink_domain,
		                   TOPOLOGY_ROLE_DIRECTORY_UPSTREAM);
	}
	if (ret == 0)
	{
		ret = dta_add_role(topology, cfg->converter_device_id, downlink_domain,
		                   TOPOLOGY_ROLE_MEDIA_GATEWAY);
	}
	if (ret == 0)
	{
		ret = dta_add_link(topology, cfg->unit_device_id,
		                   cfg->converter_device_id, downlink_domain,
		                   TOPOLOGY_LINK_SPE, capabilities);
	}
	if (ret == 0)
	{
		ret = dta_add_endpoint(topology, cfg->unit_device_id,
		                       downlink_domain, TOPOLOGY_LINK_SPE,
		                       cfg->unit_downlink_addr, cfg->downlink_port);
	}
	if (ret == 0)
	{
		ret = dta_add_endpoint(topology, cfg->converter_device_id,
		                       downlink_domain, TOPOLOGY_LINK_SPE,
		                       cfg->converter_downlink_addr, cfg->downlink_port);
	}
	if (ret != 0)
	{
		return ret;
	}

	return simple_topology_profile_add_plc_cco(topology, &cco);
}

int simple_topology_profile_find_local_endpoint(topology_ctx_t *topology,
                                                const char *local_device_id,
                                                const topology_endpoint_t *peer,
                                                topology_endpoint_t *local_out)
{
	topology_endpoint_t ep;
	size_t i;

	if (!topology || !IS_VALID_STRING(local_device_id) || !peer || !local_out)
	{
		return -EINVAL;
	}

	for (i = 0; topology_get_endpoint_at(topology, i, &ep) == 0; i++)
	{
		if (strcmp(ep.device_id, local_device_id) == 0 &&
		    strcmp(ep.domain_id, peer->domain_id) == 0 &&
		    ep.link_kind == peer->link_kind &&
		    ep.addr != 0 && ep.port != 0)
		{
			*local_out = ep;
			return 0;
		}
	}
	return -ENOENT;
}


int simple_topology_profile_prepare_audio(const simple_topology_profile_prepare_audio_t *cfg)
{
	topology_audio_query_t query;
	topology_audio_endpoint_result_t audio;
	char local_device_id[TOPOLOGY_DEVICE_ID_MAX];
	char peer_device_id[TOPOLOGY_DEVICE_ID_MAX];
	char line[1024];
	int ret;

	if (!cfg)
	{
		return -EINVAL;
	}
	if (!cfg->topology)
	{
		dta_log_fail(cfg, -EINVAL, "no_topology", cfg->local_device_id, cfg->peer_device_id);
		return -EINVAL;
	}
	if (dta_copy(local_device_id, sizeof(local_device_id), cfg->local_device_id) < 0)
	{
		dta_log_fail(cfg, -EINVAL, "local_device_id", cfg->local_device_id, cfg->peer_device_id);
		return -EINVAL;
	}

	if (IS_VALID_STRING(cfg->peer_device_id))
	{
		ret = dta_copy(peer_device_id, sizeof(peer_device_id), cfg->peer_device_id);
	}
	else if (IS_VALID_STRING(cfg->caller_device_id) &&
	         strcmp(local_device_id, cfg->caller_device_id) == 0)
	{
		ret = dta_copy(peer_device_id, sizeof(peer_device_id), cfg->callee_device_id);
	}
	else if (IS_VALID_STRING(cfg->callee_device_id) &&
	         strcmp(local_device_id, cfg->callee_device_id) == 0)
	{
		ret = dta_copy(peer_device_id, sizeof(peer_device_id), cfg->caller_device_id);
	}
	else
	{
		ret = -EINVAL;
	}
	if (ret < 0)
	{
		dta_log_fail(cfg, ret, "peer_device_id", local_device_id, cfg->peer_device_id);
		return ret;
	}

	memset(&query, 0, sizeof(query));
	if (IS_VALID_STRING(cfg->caller_device_id) &&
	    strcmp(local_device_id, cfg->caller_device_id) == 0)
	{
		query.session_role = TOPOLOGY_AUDIO_SESSION_CALLER;
	}
	else if (IS_VALID_STRING(cfg->callee_device_id) &&
	         strcmp(local_device_id, cfg->callee_device_id) == 0)
	{
		query.session_role = TOPOLOGY_AUDIO_SESSION_CALLEE;
	}
	else
	{
		query.session_role = TOPOLOGY_AUDIO_SESSION_RELAY;
	}

	ret = topology_query_audio_endpoint(cfg->topology, local_device_id, peer_device_id,
	                                    &query, &audio);
	if (ret < 0)
	{
		dta_log_fail(cfg, ret, "query", local_device_id, peer_device_id);
		return ret;
	}
	if (cfg->audio_out)
	{
		*cfg->audio_out = audio;
	}
	if (cfg->expected_tx_link_kind != TOPOLOGY_LINK_UNKNOWN &&
	    audio.tx_endpoint.link_kind != cfg->expected_tx_link_kind)
	{
		dta_log_fail(cfg, -EINVAL, "link_mismatch", local_device_id, peer_device_id);
		return -EINVAL;
	}
	if (cfg->validate_tx_endpoint &&
	    (audio.tx_endpoint.addr != cfg->expected_tx_addr ||
	     audio.tx_endpoint.port != cfg->expected_tx_port))
	{
		dta_log_fail(cfg, -EINVAL, "tx_endpoint_mismatch",
		             local_device_id, peer_device_id);
		return -EINVAL;
	}
	if (cfg->validate_rx_port && audio.rx_match_endpoint.port != cfg->expected_rx_port)
	{
		dta_log_fail(cfg, -EINVAL, "rx_endpoint_mismatch",
		             local_device_id, peer_device_id);
		return -EINVAL;
	}

	snprintf(line, sizeof(line),
	         "topology audio endpoint applied: node=%s local=%s peer=%s "
	         "tx_device=%s tx_domain=%s tx_link=%d tx_addr=0x%x tx_port=%u "
	         "rx_addr=0x%x rx_port=%u bridge_required=%u gateway=%s "
	         "bridge_peer_domain=%s bridge_peer_addr=0x%x session=0x%llx msg=0x%02x "
	         "explain=\"%s\"",
	         cfg->node_name ? cfg->node_name : "-",
	         local_device_id,
	         peer_device_id,
	         audio.tx_endpoint.device_id,
	         audio.tx_endpoint.domain_id,
	         (int)audio.tx_endpoint.link_kind,
	         audio.tx_endpoint.addr,
	         (unsigned int)audio.tx_endpoint.port,
	         audio.rx_match_endpoint.addr,
	         (unsigned int)audio.rx_match_endpoint.port,
	         audio.bridge_required ? 1U : 0U,
	         audio.gateway_device_id[0] ? audio.gateway_device_id : "-",
	         audio.bridge_peer_endpoint.domain_id,
	         audio.bridge_peer_endpoint.addr,
	         (unsigned long long)cfg->session_id,
	         (unsigned int)cfg->msg_type,
	         audio.explain);
	dta_log(cfg, line);
	return 0;
}
