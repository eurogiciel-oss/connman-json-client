#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <json/json.h>

#include "json_utils.h"

#define IPV6_REGEX "^((([0-9A-Fa-f]{1,4}:){7}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}:[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){5}:([0-9A-Fa-f]{1,4}:)?[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){4}:([0-9A-Fa-f]{1,4}:){0,2}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){3}:([0-9A-Fa-f]{1,4}:){0,3}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){2}:([0-9A-Fa-f]{1,4}:){0,4}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){6}((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|(([0-9A-Fa-f]{1,4}:){0,5}:((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|(::([0-9A-Fa-f]{1,4}:){0,5}((b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b).){3}(b((25[0-5])|(1d{2})|(2[0-4]d)|(d{1,2}))b))|([0-9A-Fa-f]{1,4}::([0-9A-Fa-f]{1,4}:){0,5}[0-9A-Fa-f]{1,4})|(::([0-9A-Fa-f]{1,4}:){0,6}[0-9A-Fa-f]{1,4})|(([0-9A-Fa-f]{1,4}:){1,7}:))$"

#define IPV4_REGEX "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"

int main()
{
	/*
	const char *to_test[] = { 
			"{ \"key\": \"labbcc\", \"key2\": [ \"cc\", \"aa\", \"bb\" ]}",
			"{ \"key\": \"pabbcc\", \"key2\": [ 2 ]}",
			"{ \"key\": \"babbcc\", \"key2\": [ ]}",
			"{ \"key\": \"aabbcc\", \"key2\": [ { \"key3\": \"zz\" ]}",
			"{ \"not_the_key\": \"aabbcc\" }",
			"{ \"key\": 3 }",
			"",
			NULL,
			};
	const char *trusted = "{\"key\": \"[[:alpha:]]\", \"key2\": [\"[[:alpha:]]\"]}";

	const char *to_test[] = {
			"{ \"options\": { \"IPv4\": { \"Address\": \"12.12.12.12\" } } }",
			"{ \"options\": { \"IPv4\": { \"Address\": \"999.12.12.12\" } } }",
			"{ \"options\": { \"IPv4\": { \"Address\": \"...\" } } }",
			"{ \"options\": { \"IPv6\": { \"Address\": \"2001:41D0:1:2E4e::1\" } } }",
			"{ \"options\": { \"IPv6\": { \"Address\": \"2001::1:2E4e::1\" } } }",
			"{ \"options\": { \"IPv6\": { \"Address\": \"::1:::2\" } } }",
			NULL,
			};
	*/
	/*
	const char *to_test[] = {
		"{ \"Identity\" : \"bob\", \"Passphrase\": \"secret123\" }",
		"{ \"Identity\" : \"bob\", \"Passphrase\": \"123\" }",
		"{ \"Identity\" : \"bob\" }",
		"{ \"Passphrase\": \"123\" }",
		"{ \"Identity\" : \"b o b\", \"Passphrase\": \"secr[}et123\" }",
		"{ \"Identity\" : \"b?o!b\", \"Passphrase\": \"secret123\" }",
		"{ \"Identity\" : \"\\b?o!b\", \"Passphrase\": \"//secret123\" }",
		NULL
	};
	*/

	const char *to_test[] = {
		"{ \"Method\" : \"auto\" }",
		"{ \"Method\" : \"manual\" }",
		"{ \"Method\" : \"6to4\" }",
		"{ \"Method\" : \"off\" }",
		"{ \"Method\" : \"au to\" }",
		"{ \"Method\" : \"auto \" }",
		"{ \"Method\" : \"klfjsdlksdfjlskdfj\" }",
		NULL
	};
 
	/*
	const char *trusted = "{ \"service\": \"wifi_8888_8888_none\", \
	\"options\": { \
		\"IPv4\": { \
			\"Address\": \"([[:digit:]]{1,3}\\.){3}\\.[[:digit:]]{1,3}\" \
		} \
	}";
	*/
	//struct json_object *jtmp, *jtrusted, *jsubtrusted, *jsubtrusted2, *jsubtrusted3;
	struct json_object *jtmp, *jtrusted;
	int i;

	printf("\n[*] start\n");

	//jtrusted = json_tokener_parse_ex(tok, trusted, strlen(trusted));
	//printf("\n[*] jtok res: %s\n", json_tokener_error_desc(json_tokener_get_error(tok)));

	jtrusted = json_object_new_object();
	/*
	jsubtrusted = json_object_new_object();
	jsubtrusted2 = json_object_new_object();
	jsubtrusted3 = json_object_new_object();

	json_object_object_add(jsubtrusted2, "Address", json_object_new_string(IPV4_REGEX));
	json_object_object_add(jsubtrusted, "IPv4", jsubtrusted2);
	json_object_object_add(jsubtrusted3, "Address", json_object_new_string(IPV6_REGEX));
	json_object_object_add(jsubtrusted, "IPv6", jsubtrusted3);
	json_object_object_add(jtrusted, "options", jsubtrusted);
	*/

	/*
	json_object_object_add(jtrusted, "Identity",
			json_object_new_string("^([[:alpha:]]+)$"));
	json_object_object_add(jtrusted, "Passphrase",
			json_object_new_string("^(([[:alpha:]]|[[:digit:]])*)$"));
	*/

	json_object_object_add(jtrusted, "Method",
			json_object_new_string("^(auto|manual|6to4|off)$"));

	for (i = 0; to_test[i]; i++) {

		jtmp = json_tokener_parse(to_test[i]);
		printf("\n[*] test %d ... ", i);

		if (__json_type_dispatch(jtmp, jtrusted))
			printf("PASSED");
		else
			printf("FAILED");

		printf("\n---\n%s\n---\n", to_test[i]);

		json_object_put(jtmp);
		jtmp = NULL;
	}

	printf("\n[*] the end.\n");
	json_object_put(jtrusted);

	return 0;
}
