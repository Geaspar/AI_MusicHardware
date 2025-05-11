#include <json/json.h>
#include <stdio.h>

int main() {
  /*Creating a json object*/
  json_object * jobj = json_object_new_object();
  /*Creating a json double*/
  json_object *jdouble = json_object_new_double(3.14);
  /*Form the json object*/
  json_object_object_add(jobj,"PI", jdouble);
  /*Now printing the json object*/
  printf ("The json object created: %sn",json_object_to_json_string(jobj));
}
