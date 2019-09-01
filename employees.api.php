<?php
/**
 * Get info for employees API Url
 *
 * @return array Employees Array
 */
function get_api_employees()
{
    $service_url = variable_get('employees_webserviceurl', 'http://dummy.restapiexample.com/api/v1/employees');
    
    $request = drupal_http_request($service_url);
  
    $employees_response =  drupal_json_decode($request->data);
  
    return $employees_response;
}