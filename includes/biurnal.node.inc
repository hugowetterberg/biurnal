<?php
// $Id$
/**
 * @file
 * Implements the Biurnal configuration node
 * TODO: Deprecate and/or make sure this module works with Drupal 6.x
 */

function biurnal_node_info() {
  return array(
    'biurnal' => array(
      'name'        => t('Biurnal configuration node'),
      'module'      => 'biurnal',
      'description' => t('This is a configuration node for biurnal'),
    )
  );
}

function biurnal_access($op, $node, $account) {
  if ($op == 'create') {
    return user_access('create biurnal node', $account);
  }

  if ($op == 'update' || $op == 'delete') {
    if (user_access('edit all biurnal nodes', $account) || (user_access('edit own biurnal node', $account) && ($account->uid == $node->uid))) {
      return TRUE;
    }
  }
}

function biurnal_form(&$node) {
  $type = node_get_types('type', $node);
  $base = drupal_get_path('module', 'biurnal');

  $form['title'] = array(
    '#type'          => 'textfield',
    '#title'         => check_plain($type->title_label),
    '#required'      => TRUE,
    '#default_value' => $node->title,
    '#weight'        => -5
  );

  $scheme = biurnal_get_color_scheme();
  if (isset($node->palette)) {
    $scheme = array_merge($scheme, $node->palette);
  }
  if (!empty($scheme)) {
    // Add Farbtastic color picker
    drupal_add_css('misc/farbtastic/farbtastic.css', 'module', 'all', FALSE);
    drupal_add_js('misc/farbtastic/farbtastic.js');

    // Add custom CSS/JS
    drupal_add_css($base .'/biurnal.css', 'module', 'all', FALSE);
    drupal_add_js($base .'/biurnal.js');

    $form['biurnal_color'] = array(
      '#type'       => 'fieldset',
      '#title'      => t('Color scheme'),
      '#weight'     => -1,
      '#attributes' => array('class' => 'biurnal-color-scheme-form'),
      '#theme'      => 'biurnal_scheme_form',
    );
    $form['biurnal_color_present'] = array(
      '#type'  => 'hidden',
      '#value' => TRUE,
    );
    $form['biurnal_color']['scheme'] = biurnal_scheme_form($scheme);
  }

  return $form;
}

function biurnal_update(&$node) {
  biurnal_insert($node, TRUE);
}

/**
 * Gets the default scheme for the installation.
 *
 * @return array
 */
function biurnal_get_color_scheme() {
  $biurnal = Biurnal::instance();
  $themes = list_themes();
  $scheme = array();
  foreach ($themes as $theme) {
    if ($theme->status && Biurnal::instance()->theme_is_biurnal($theme->name)) {
      $scheme = array_merge($scheme, $biurnal->get_colors_for_theme($theme->name));
    }
  }
  return $scheme;
}

function biurnal_insert(&$node, $is_update = FALSE) {
  $biurnal = Biurnal::instance();
  $scheme = biurnal_get_color_scheme();

  if (!isset($node->palette)) {
    $node->palette = array();
  }
  foreach ($scheme as $name => $color) {
    $attribute = 'biurnal_color_' . $name;
    if (isset($node->$attribute)) {
      $node->palette[$name] = $node->$attribute;
    }
  }

  if (!$is_update) {
    db_query("INSERT INTO {biurnal_configuration}(nid, configuration) VALUES (%d, '%s')", array(
      ':nid' => $node->nid, 
      ':configuration' => serialize($node->palette),
    ));
  }
  else {
    db_query("UPDATE {biurnal_configuration} AS bc 
      SET bc.configuration = '%s' 
      WHERE bc.nid = %d", array(
        ':configuration' => serialize($node->palette), 
        ':nid' => $node->nid,
    ));
  }
}

function biurnal_load(&$node) {
  $node->palette = biurnal_get_color_scheme();
  //Fetch config from database
  $bc = db_fetch_object(db_query('SELECT configuration FROM {biurnal_configuration} WHERE nid = %d', $node->nid));
  if ($bc) {
    $node->palette = array_merge($node->palette, unserialize($bc->configuration));
  }

  if (Biurnal::instance()->preview_scheme() == $node->nid) {
    $node->body = l(t('Stop previewing scheme'), 'biurnal/stop_preview');
  }
  else {
    $node->body = l(t('Preview scheme'), 'biurnal/preview/' . $node->nid);
  }
}

function biurnal_scheme_form($palette) {
  $form = array();

  // Add palette fields
  $names = array(
    'main'       => t('Base color'),
    'background' => t('Background color'),
    'link'       => t('Link color'),
    'top'        => t('Header top'),
    'bottom'     => t('Header bottom'),
    'text'       => t('Text color')
  );

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

/**
 * Theme color form.
 */
function theme_biurnal_scheme_form($form) {
  // Include stylesheet
  $theme = $form['theme']['#value'];
  $info  = $form['info']['#value'];
  $path  = drupal_get_path('theme', $theme) . '/';

  // Wrapper
  $output .= '<div class="biurnal-form clear-block">';

  // Palette
  $output .= '<div class="palette">';
  foreach (element_children($form['palette']) as $name) {
    $output .= drupal_render($form['palette'][$name]);
  }
  $output .= '</div>';

  // Preview
  $output .= drupal_render($form);

  // Close wrapper
  $output .= '</div>';

  return $output;
}