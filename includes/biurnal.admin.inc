<?php

// $Id$

function biurnal_settings_form($form_state) {
  $form = array();

  $form['active_configuration'] = array(
    '#type' => 'select',
    '#title' => t('Active configuration'),
    '#description' => t('The biurnal scheme that should be used on the site'),
    '#options' => array(
      0 => 'Select a biurnal configuration',
    ),
    '#default_value' => variable_get('biurnal_active_configuration', 0),
  );

  $res = db_query("SELECT id, title FROM {biurnal_configuration} ORDER BY title");
  while ($conf = db_fetch_array($res)) {
    $form['active_configuration']['#options'][$conf['id']] = $conf['title'];
  }

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

function biurnal_settings_form_submit($form, $form_state) {
  variable_set('biurnal_active_configuration', $form_state['values']['active_configuration']);
}

function biurnal_list_configurations_form($form_state) {
  $form = array();

  $res = db_query("SELECT id, title FROM {biurnal_configuration} ORDER BY title");
  while ($conf = db_fetch_array($res)) {
    $form[$conf['id']] = array(
      '#type' => 'item',
      '#title' => $conf['title'],
      '#value' => l(t('Edit'), 'admin/settings/biurnal/edit/' . $conf['id']) . ' ' .
        l(t('Delete'), 'admin/settings/biurnal/delete/' . $conf['id']),
    );
  }

  return $form;
}

function biurnal_edit_configuration_form($form_state, $configuration) {
  $form = array();

  $biurnal = Biurnal::instance();
  $biurnal->bootstrap();
  $scheme = $biurnal->get_colors_hex();
  $scheme = array_merge($scheme, $configuration->configuration);

  // Add Farbtastic color picker
  drupal_add_css('misc/farbtastic/farbtastic.css', 'module', 'all', FALSE);
  drupal_add_js('misc/farbtastic/farbtastic.js');

  // Add custom CSS/JS
  $base = drupal_get_path('module', 'biurnal');
  drupal_add_css($base .'/biurnal.css', 'module', 'all', FALSE);
  drupal_add_js($base .'/biurnal.js');
  
  $form['configuration_id'] = array(
    '#type' => 'value',
    '#value' => $configuration->id,
  );

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('The title of the configuration'),
    '#size' => 30,
    '#required' => TRUE,
    '#maxlength' => 255,
    '#default_value' => $configuration->title,
  );

  $form['biurnal_color'] = array(
    '#type'       => 'fieldset',
    '#title'      => t('Color scheme'),
    '#attributes' => array('class' => 'biurnal-color-scheme-form'),
    '#theme'      => 'biurnal_scheme_form',
  );
  $form['biurnal_color']['scheme'] = biurnal_scheme_form($scheme);

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

function biurnal_edit_configuration_form_submit($form, $form_state) {
  $values = $form_state['values'];
  $biurnal = Biurnal::instance();
  $conf = biurnal_configuration_load($values['configuration_id']);
  
  $biurnal->bootstrap();
  $scheme = $biurnal->get_colors_hex();
  foreach ($scheme as $name => $color) {
    $field = 'biurnal_color_' . $name;
    if (!empty($values[$field])) {
      $conf->configuration[$name] = $values[$field];
    }
  }

  db_query("UPDATE {biurnal_configuration} AS bc 
    SET bc.title = '%s', bc.configuration = '%s', changed=%d
    WHERE bc.id = %d", array(
      ':title' => $values['title'],
      ':configuration' => serialize($conf->configuration),
      ':changed' => time(),
      ':nid' => $conf->id,
  ));
}

function biurnal_delete_configuration_form($form_state, $conf) {
  $form = array();

  $form['configuration_id'] = array(
    '#type' => 'value',
    '#value' => $conf->id,
  );

  $form['text'] = array(
    '#type' => 'item',
    '#value' => t('Do you really want to delete @name?', array(
      '@name' => $conf->title,
    )),
  );

  $form['yes'] = array(
    '#type' => 'submit',
    '#value' => t('Yes'),
  );

  $form['no'] = array(
    '#type' => 'item',
    '#value' => l(t('No'), 'admin/settings/biurnal/configurations'),
  );

  return $form;
}

function biurnal_delete_configuration_form_submit($form, $form_state) {
  $values = $form_state['values'];
  db_query("DELETE FROM {biurnal_configuration} WHERE id=%d", array(
    ':id' => $values['configuration_id'],
  ));
  drupal_goto('admin/settings/biurnal/configurations');
}

function biurnal_add_configuration_form($form_state) {
  $form = array();

  $biurnal = Biurnal::instance();
  $biurnal->bootstrap();
  $scheme = $biurnal->get_colors_hex();

  // Add Farbtastic color picker
  drupal_add_css('misc/farbtastic/farbtastic.css', 'module', 'all', FALSE);
  drupal_add_js('misc/farbtastic/farbtastic.js');

  // Add custom CSS/JS
  $base = drupal_get_path('module', 'biurnal');
  drupal_add_css($base .'/biurnal.css', 'module', 'all', FALSE);
  drupal_add_js($base .'/biurnal.js');

  $form['title'] = array(
    '#type' => 'textfield',
    '#title' => t('Title'),
    '#description' => t('The title of the configuration'),
    '#size' => 30,
    '#required' => TRUE,
    '#maxlength' => 255,
    '#default_value' => $configuration->title,
  );

  $form['biurnal_color'] = array(
    '#type'       => 'fieldset',
    '#title'      => t('Color scheme'),
    '#attributes' => array('class' => 'biurnal-color-scheme-form'),
    '#theme'      => 'biurnal_scheme_form',
  );
  $form['biurnal_color']['scheme'] = biurnal_scheme_form($scheme);

  $form['save'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

function biurnal_add_configuration_form_submit($form, $form_state) {
  $values = $form_state['values'];
  $biurnal = Biurnal::instance();
  $biurnal->bootstrap();

  $scheme = $biurnal->get_colors_hex();
  foreach ($scheme as $name => $color) {
    $field = 'biurnal_color_' . $name;
    if (!empty($values[$field])) {
      $scheme[$name] = $values[$field];
    }
  }

  db_query("INSERT INTO {biurnal_configuration}(title, configuration, changed) 
    VALUES('%s', '%s', %d)", array(
      ':title' => $values['title'],
      ':configuration' => serialize($scheme),
      ':changed' => time(),
  ));
  drupal_goto('admin/settings/biurnal');
}

function biurnal_scheme_form($palette) {
  $form = array();

  // Import field names from theme.biurnal.inc
  $theme = variable_get('theme_default', '');
  if($theme) {
    $biurnal_file = drupal_get_path('theme', $theme) . '/' . $theme . '.biurnal.inc';
    require_once($biurnal_file);
    if (function_exists($theme . '_biurnal_data')) {
      $scheme = call_user_func($theme . '_biurnal_data');
      $names = array();
      foreach ($scheme['colors'] as $key => $value) {
        foreach ($value as $name => $title) {
          if($name == 'title') {
            $names[$key] = $title;
          }
        }
      }
    }
  }

  foreach ($palette as $name => $value) {
    $form['biurnal_color_' . $name] = array(
      '#type'          => 'textfield',
      '#title'         => str_replace('_', ' ', isset($names[$name])?$names[$name]:$name),
      '#default_value' => $value,
      '#size'          => 8,
    );
  }

  return $form;
}