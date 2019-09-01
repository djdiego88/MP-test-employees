<?php
/**
 * Settings Form
 *
 * @param [type] $form
 * @param [type] $form_state
 * @return Form Structure
 */
function employees_settings_form($form, &$form_state)
{
    $form['employees_webserviceurl'] = array(
        '#type' => 'textfield',
        '#title' => t('URL del servicio Web'),
        '#default_value' => variable_get('employees_webserviceurl', 'http://dummy.restapiexample.com/api/v1/employees'),
        '#required' => true,
        '#description' => t('Ejemplo: ') . 'http://dummy.restapiexample.com/api/v1/employees',
    );

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Importar Empleados'),
    );

    return $form;
}
/**
 * Submit Callback - Batch Import for Employees
 *
 * @param [type] $form
 * @param [type] $form_state
 * @return void
 */
function employees_settings_form_submit(&$form, &$form_state) 
{
    if (isset($form_state['values']['employees_webserviceurl']) ) {
        if (variable_get('employees_webserviceurl', '') != $form_state['values']['employees_webserviceurl']) {
            variable_set('employees_webserviceurl', $form_state['values']['employees_webserviceurl']);
        }
        
        $batch = [
            'title' => t('Importando Empleados ...'),
            'operations' => [],
            'init_message' => t('Iniciando'),
            'progress_message' => t('Importados @current de @total.'),
            'error_message' => t('Un error ha ocurrido durante la importación'),
            'finished' => 'employees_import_finished',
        ];

        $employees = get_api_employees();

        foreach ($employees as $employee) {
            $batch['operations'][] = ['employee_import_progress', [$employee]];
        }

        batch_set($batch);
        batch_process('admin/config/content/employees');
    }
}
/**
 * Employee Import Progress
 *
 * @param [type] $employee
 * @param [type] $context
 * @return void
 */
function employee_import_progress($employee, &$context)
{
    /**
     * Check if exists employee in DB
     */
    $query = new EntityFieldQuery();
    $query->entityCondition('entity_type', 'node')
        ->entityCondition('bundle', 'employees')
        ->propertyCondition('status', NODE_PUBLISHED)
        ->fieldCondition('field_remote_id', 'value', $employee['id'], '=')
        ->addMetaData('account', user_load(1));
    $result = $query->execute();
    if (!empty($result['node'])) {
        $employee_nid = array_keys($result['node']);
        update_employee($employee, $employee_nid[0]);
        $context['message'] = t('Actualizando %name', ['%name' => $employee['employee_name']]);
    } else {
        save_employee($employee);
        $context['message'] = t('Importando %name', ['%name' => $employee['employee_name']]);
    }
    
}
/**
 * Finished Import Progress
 *
 * @param [type] $success
 * @param [type] $results
 * @param [type] $operations
 * @return void
 */
function employees_import_finished($success, $results, $operations)
{
    if ($success) {
        drupal_set_message(t('¡Se ha completado la importación de los empleados!'));
    }
    else {
        $error_operation = reset($operations);
        $message = t(
            'An error occurred while processing %error_operation with arguments: @arguments', array(
            '%error_operation' => $error_operation[0],
            '@arguments' => print_r($error_operation[1], true)
            )
        );
        drupal_set_message($message, 'error');
    }
}
/**
 * Create a new employee
 *
 * @param array $employee Employee info
 * @return void
 */
function save_employee($employee)
{
    global $user;

    $node = new stdClass();
    $node->title = $employee['employee_name'];
    $node->type = 'employees';
    node_object_prepare($node);
    $node->language = LANGUAGE_NONE;
    $node->uid = $user->uid; 
    $node->status = 1;
    $node->promote = 0;
    $node->comment = 1;      
    $node->field_remote_id[$node->language][]['value'] = $employee['id'];
    $node->field_age[$node->language][]['value'] = $employee['employee_age'];
    $node->field_salary_type[$node->language][]['tid'] = get_salary_type_tid($employee['employee_salary']);

    $node = node_submit($node);
    node_save($node);
}
/**
 * Update an employee
 *
 * @param array $employee
 * @param integer $nid
 * @return void
 */
function update_employee($employee, $nid)
{
    global $user;

    $node = node_load($nid);
    $node->language = LANGUAGE_NONE;
    $node->uid = $user->uid; 
    $node->status = 1;
    $node->promote = 0;
    $node->comment = 1;      
    $node->field_remote_id[$node->language][]['value'] = $employee['id'];
    $node->field_age[$node->language][]['value'] = $employee['employee_age'];
    $node->field_salary_type[$node->language][]['tid'] = get_salary_type_tid($employee['employee_salary']);
    node_save($node);
}
/**
 * Get salary term ID based on Salary from API
 *
 * @param integer $salary
 * @return integer Term ID
 */
function get_salary_type_tid($salary)
{
    $terms_sb = taxonomy_get_term_by_name('Salario bajo', 'salary_type');
    $tid_sb = key($terms_sb);
    $terms_sm = taxonomy_get_term_by_name('Salario medio', 'salary_type');
    $tid_sm = key($terms_sm);
    $terms_sa = taxonomy_get_term_by_name('Salario alto', 'salary_type');
    $tid_sa = key($terms_sa);

    $salary = (int) $salary;

    if ($salary <= 1000 ) {
        return $tid_sb;
    } else if ($salary > 1000 && $salary <= 4000 ) {
        return $tid_sm;
    } else if ($salary > 4000 ) {
        return $tid_sa;
    }
}