#include <json/json.h>
#include <stdio.h>

void json_parse(json_object * jobj) {
 enum json_type type;
 json_object_object_foreach(jobj, key, val) {
 type = json_object_get_type(val);
 switch (type) {
   case json_type_double:
//   printf("type: json_type_double, ");
//   printf("value: %fn", json_object_get_double(val));
   return json_object_get_double(val); 
//   break;
 }
 }
}

// int main() {
// char * string = "{ \"PI\" : 3.140000 }";
// printf ("JSON string is: %s\n", string);
// json_object * jobj = json_tokener_parse(string);
// json_parse(jobj);
// }
//
