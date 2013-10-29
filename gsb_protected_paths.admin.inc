<?php
/**
 * @file
 * Provides callbacks for configuration page.
 */
/**
 * Form for add protected paths.
 */
function gsb_protected_paths_add($form, &$form_state) {

  $form['alias_list'] = array(
    '#title' => t('Add Protected Path.'),
    '#type' => 'fieldset',
    '#description' => t('Enter the protected path and password.
     For example, "programs/phd" will protect only phd page and
     "programs/phd/*" will protect level 1 pages below phd.'),
  );
  $form['alias_list']['path'] = array(
    '#type' => 'textarea',
    '#title' => t('Paths'),
    '#description' => t('Enter paths to be protected. For example, "programs/phd", "programs/phd/*" etc.'),
    '#required' => TRUE,
  );
  $form['alias_list']['password_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Password Screen Title'),
    '#description' => t('Enter password screen text.'),
  );
  $form['alias_list']['password'] = array(
    '#type' => 'password_confirm',
    '#size' => 25,
    '#required' => TRUE,
  );
  $form['alias_list']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

/**
 * Form for list protected paths.
 */
function gsb_protected_paths_form($form, &$form_state) {

  $form['intro'] = array(
    '#markup' => '<p>' . t('List of all the protected paths.') . '</p>',
  );

  // Build the table rows.
  $query = db_select('gsb_protected_paths', 'pp')->extend('PagerDefault')->extend('TableSort');
  $query->fields('pp')
      ->limit(20);

  $result = $query->execute();

  foreach ($result as $data) {
    $form['pid:' . $data->pid] = array(
      '#weight' => $data->weight,
      'weight' => array(
        '#type' => 'weight',
        '#default_value' => $data->weight,
      ),
      'path' => array(
        '#markup' => drupal_get_path_alias($data->path),
      ),
      'password_text' => array(
        '#markup' => $data->password_text,
      ),
      'pid' => array(
      '#type' => 'hidden',
        '#default_value' => $data->pid,
      ),
      'operations' => array(
        'edit' => array(
          '#type' => 'link',
          '#title' => t('Edit'),
          '#href' => 'admin/config/gsb/gsb_protected_paths/' . $data->pid . '/edit',
        ),
        'delete' => array(
          '#type' => 'link',
          '#title' => t('Delete'),
          '#href' => 'admin/config/gsb/gsb_protected_paths/' . $data->pid . '/delete',
        ),
      ),
    );
  } 

  $form['#tree'] = TRUE;

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  return $form;
}

function gsb_protected_paths_form_submit(&$form, &$form_state) {

  $values = array();
  $query = db_select('gsb_protected_paths')
      ->fields('gsb_protected_paths', array('path', 'pid'));
  $result = $query->execute();

  foreach ($result as $record) {
    if ($form_state['values']['pid:' . $record->pid]['pid'] = $record->pid) {
      $values['weight'] = $form_state['values']['pid:' . $record->pid]['weight'];
      db_update('gsb_protected_paths')
        ->fields($values)
        ->condition('pid', $record->pid)
        ->execute(); 
    }
  }

  drupal_set_message(t('Your configuration is saved'));
}

function theme_gsb_protected_paths_form($variables) {

  $form = $variables['form'];

  $output = drupal_render($form['intro']);

  drupal_add_tabledrag('gsb-protected-paths-order', 'order', 'sibling', 'gsb-protected-paths-order-weight');
  $header = array(
     t('Paths'),
     t('Password Text'),
     t('Weight'),
     array(
      'data' => t('Operations'),
      'colspan' => 2,
      'width' => '30%',
    ),
  );

  // Build the form element.
  $rows = array();
  foreach (element_children($form) as $item) {
    $element = &$form[$item];

    // Build a list of operations.
    $operations = array(drupal_render($element['operations']));

    // Add special class to be used with tabledrag.js
    if (isset($element['weight'])) {
      $element['weight']['#attributes']['class'] = array('gsb-protected-paths-order-weight');
    }

    if (isset($element['pid'])) {
      $row = array();
      $row[] = drupal_render($element['path']);
      $row[] = drupal_render($element['password_text']);
      $row[] = drupal_render($element['weight']);
      $row = array_merge($row, $operations);
      $row = array('data' => $row);
      $row['class'][] = 'draggable';
      $rows[] = $row;
    }
  }

  $output .= theme_table(
          array(
            "header" => $header,
            "rows" => $rows,
            "attributes" => array('width' => "500", 'id' => 'gsb-protected-paths-order'),
            "sticky" => TRUE,
            "caption" => "",
            "colgroups" => array(),
            "empty" => t("No record found!"),
          )
      );
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Implements hook_validate().
 */
function gsb_protected_paths_add_validate($form, &$form_state) {
  $form_state['values']['path'] = drupal_get_path_alias(drupal_get_normal_path($form_state['values']['path']));

  $pid = db_select('gsb_protected_paths')
      ->fields('gsb_protected_paths', array('pid'))
      ->condition('path', $form_state['values']['path'])
      ->range(0, 1)
      ->execute()
      ->fetchField();
  if ($pid) {
    form_set_error('path', t('Duplicate path not allowed.'));
  } 
}

/**
 * Ajax submit callback for add protected paths form.
 */
function gsb_protected_paths_add_submit($form, &$form_state) {
  $errors = form_get_errors();

  if (count($errors) < 1) {
    db_insert('gsb_protected_paths')->fields(array(
      'password' => sha1(trim(check_plain($form_state['values']['password']))),
      'path' => drupal_get_path_alias(check_plain($form_state['values']['path'])),
      'password_text' => check_plain($form_state['values']['password_text']),
      'weight' => 0,
    ))->execute();

    drupal_set_message(t('The protected paths settings has been successfully saved.'));
    $form_state['redirect'] = 'admin/config/gsb/gsb_protected_paths';
  }
}


/**
 * Callback function for edit protected paths form.
 */
function gsb_protected_paths_edit($form, &$form_state, $pid) {

  $protected_paths = db_select('gsb_protected_paths')
      ->fields('gsb_protected_paths', array('path', 'password_text'))
      ->condition('pid', $pid)
      ->execute()
      ->fetchObject();

  $form['alias_list'] = array(
    '#title' => t("Edit Protected Paths."),
    '#type' => 'fieldset',
    '#prefix' => '<div id="alias_list">',
    '#suffix' => '</div>',
    '#description' => t('Enter the protected paths and password.
     For example, "programs/phd" will protect only phd page and
     "programs/phd/*" will protect level 1 pages below phd.'),
  );
  $form['alias_list']['path'] = array(
    '#type' => 'textarea',
    '#title' => t('Paths'),
    '#description' => t('Enter paths to be protected. For example, "programs/phd", "programs/phd/*" etc.'),
    '#required' => TRUE,
    '#default_value' => $protected_paths->path,
  );
  $form['alias_list']['password_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Password Screen Text'),
    '#description' => t('Enter password screen text.'),
    '#required' => TRUE,
    '#default_value' => $protected_paths->password_text,
  );
  $form['alias_list']['password'] = array(
    '#type' => 'password_confirm',
    '#size' => 25,
  );
  $form['alias_list']['pid'] = array(
    '#type' => 'hidden',
    '#value' => $pid,
  );
  $form['alias_list']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  return $form;
}

/**
 * Implements hook_submit().
 */
function gsb_protected_paths_edit_submit($form, &$form_state) {

  $values['path'] = check_plain($form_state['values']['path']);
  $values['password_text'] = check_plain($form_state['values']['password_text']);

  if (!empty($form_state['values']['password'])) {
    $values['password'] = sha1(trim(check_plain($form_state['values']['password'])));
  }
  db_update('gsb_protected_paths')
      ->fields($values)
      ->condition('pid', $form_state['values']['pid'])
      ->execute();
  drupal_set_message(t('The protected paths settings has been successfully saved.'));
  $form_state['redirect'] = 'admin/config/gsb/gsb_protected_paths';
}

/**
 * Callback function for delete protected paths.
 */
function gsb_protected_paths_delete_confirm($form, &$form_state, $pid) {

  $path = db_select('gsb_protected_paths')
      ->fields('gsb_protected_paths', array('path'))
      ->condition('pid', $pid)
      ->range(0, 1)
      ->execute()
      ->fetchField();

  $form['pid'] = array('#type' => 'hidden', '#value' => $pid);
  return confirm_form($form, t('Are you sure you want to delete <b>"%path"</b> from protected paths list?', array('%path' => $path)), 'admin/config/gsb/gsb_protected_paths', t('This action cannot be undone.'), t('Delete'), t('Cancel')
  );
}

/**
 * Implements hook_submit().
 */
function gsb_protected_paths_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $pid = $form_state['values']['pid'];

    db_delete('gsb_protected_paths')
        ->condition('pid', $pid)
        ->execute();

    drupal_set_message(t('The path has been successfully deleted from protected paths.'));
    $form_state['redirect'] = 'admin/config/gsb/gsb_protected_paths';
  }
}

/**
 * Callback function for enter password paths.
 */
function gsb_protected_paths_enter_password($form, &$form_state) {

  $query = db_select('gsb_protected_paths')
      ->fields('gsb_protected_paths', array('pid', 'password_text'))
      ->condition('pid', $_GET['protected_paths'])
      ->range(0, 1);

  $result = $query->execute()->fetchAll();

  $form['protected_paths_enter_password'] = array(
    '#type' => 'fieldset',
    '#description' => t('Please enter the password to view this page.'),
    '#collapsible' => FALSE,
  );

  $form['protected_paths_enter_password']['password'] = array(
    '#type' => 'password',
    '#title' => $result[0]->password_text,
    '#size' => 20,
    '#required' => TRUE,
  );

  $form['protected_paths_pid'] = array(
    '#type' => 'hidden',
    '#value' => $_GET['protected_paths'],
  );

  $form['protected_paths_enter_password']['submit'] = array(
    '#type' => 'submit',
    '#value' => 'Enter Page',
  );

  return $form;
}

/**
 * Implements hook_validate().
 */
function gsb_protected_paths_enter_password_validate($form, &$form_state) {
  $password = sha1(trim(check_plain($form_state['values']['password'])));
  $path = drupal_get_path_alias($_GET['destination']);

  $query = db_select('gsb_protected_paths')
      ->fields('gsb_protected_paths', array('path', 'pid'))
      ->orderby('weight', 'desc');
  $result = $query->execute();

  foreach ($result as $record) {
    $pattern_match = drupal_match_path($path, $record->path);
    if ($pattern_match) {
      $matched_pid = $record->pid;
      break;
    }
  }

  $pid = db_select('gsb_protected_paths')
        ->fields('gsb_protected_paths', array('pid'))
        ->condition('password', $password)
        ->condition('pid', $matched_pid)
        ->range(0, 1)
        ->execute()
        ->fetchField();
    if (!$pid) {

      form_set_error('password', t('Invalid password!'));
    }
}

/**
 * Implements hook_submit().
 */
function gsb_protected_paths_enter_password_submit($form, &$form_state) {
   $_SESSION['_protected_paths']['passwords'][$form_state['values']['protected_paths_pid']]['request_time'] = REQUEST_TIME;
  $session_expire_time = 0;
  if ($session_expire_time) {
    $_SESSION['_protected_paths']['passwords'][$form_state['values']['protected_paths_pid']]['expire_time'] = strtotime("+{$session_expire_time} minutes");
  }
}
