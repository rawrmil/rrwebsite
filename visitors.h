#ifndef VISITORS_H
#define VISITORS_H

#include "mongoose/mongoose.h"

#define VISITOR_PENDING_SECONDS 5
#define VISITOR_ACTIVE_SECONDS 360

typedef struct Visitor {
	uint8_t cookie_id[16];
	time_t time_created;
	time_t time_last_active;
	unsigned is_pending : 1;
} Visitor;

typedef struct {
	size_t count;
	size_t capacity;
	Visitor* items;
} VisitorArray;

extern VisitorArray visitors;

Visitor* VisitorsAddPending();
Visitor* VisitorsGetByCookieID(void* cookie_id);
bool VisitorIsActive(Visitor* visitor);
void VisitorsManageUnactive();
Visitor* HTTPProcessVisitor(struct mg_http_message* hm);
Visitor* HTTPAddPendingVisitor(Nob_String_Builder* resp_headers);

#endif /* VISITORS_H */

#ifdef VISITORS_IMPLEMENTATION

VisitorArray visitors;

Visitor* VisitorsAddPending() {
	Visitor visitor = {0};
	visitor.is_pending = 1;
	visitor.time_created = time(NULL);
	visitor.time_last_active = time(NULL);
	RandomBytes(visitor.cookie_id, 16);
	uint8_t* b = visitor.cookie_id;
	MG_DEBUG(("id=%02x%02x*****%02x", b[0], b[1], b[15]));
	nob_da_append(&visitors, visitor);
	return visitors.items + visitors.count - 1;
}

Visitor* VisitorsGetByCookieID(void* cookie_id) {
	nob_da_foreach(Visitor, visitor, &visitors) {
		if (memcmp(visitor->cookie_id, cookie_id, 16) == 0)
			return visitor;
	}
	return NULL;
}

inline bool VisitorIsActive(Visitor* visitor) {
	if (time(NULL) > visitor->time_last_active+VISITOR_ACTIVE_SECONDS) {
		return false;
	}
	return true;
}

void VisitorsManageUnactive() {
	size_t i = 0;
	active_conns = 0;
	nob_da_foreach(Visitor, visitor, &visitors) {
		if (visitor->is_pending) {
			if (time(NULL) > visitor->time_created+VISITOR_PENDING_SECONDS) {
				nob_da_remove_unordered(&visitors, i);
				MG_INFO(("expired.\n"));
				return;
			}
		}
		if (!visitor->is_pending && VisitorIsActive(visitor))
			active_conns++;
		i++;
	}
}

Visitor* HTTPProcessVisitor(struct mg_http_message* hm) {
	struct mg_str* header_cookie = mg_http_get_header(hm, "Cookie");
	if (header_cookie) {
		// Process cookie
		struct mg_str var_id = mg_http_get_header_var(*header_cookie, mg_str("id"));
		if (var_id.len == 0 || var_id.len != 32) {
			MG_DEBUG(("cookie id length %d is invalid", var_id.len));
			return NULL;
		}
		uint8_t cookie_id[16];
		for (size_t i = 0; i < 16; i++) {
			int flag = sscanf(var_id.buf+i*2, "%2hhx", &cookie_id[i]);
			if (flag != 1) {
				MG_DEBUG(("parsing error."));
				return NULL;
			}
		}
		uint8_t* b = cookie_id;
		MG_DEBUG(("id=%02x%02x*****%02x", b[0], b[1], b[15]));
		// Check if exists
		Visitor* visitor = VisitorsGetByCookieID(cookie_id);
		if (visitor == NULL) {
			MG_DEBUG(("visitor not found."));
			return NULL;
		}
		MG_DEBUG(("visitor found."));
		if (visitor->is_pending) {
			MG_DEBUG(("visitor connection upgrade."));
			visitor->is_pending = 0;
		}
		visitor->time_last_active = time(NULL);
		return visitor;
	}
	return NULL;
}

Visitor* HTTPAddPendingVisitor(Nob_String_Builder* resp_headers) {
	Visitor* visitor = VisitorsAddPending();
	if (visitor == NULL) return NULL;
	nob_sb_appendf(resp_headers, "Set-Cookie: id=");
	for (size_t i = 0; i < 16; i++)
		nob_sb_appendf(resp_headers, "%02x", visitor->cookie_id[i]);
	nob_sb_appendf(resp_headers, "\n");
	return visitor;
}


#endif /* VISITORS_IMPLEMENTATION */
