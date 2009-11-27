<?php

/**
 * Provides data for biurnal
 */
function biurnaltheme_biurnal_data() {
  return array(
    'processes' => array(
      'on_background' => array(
        'background' => array(
          'clear' => array('$background'),
        ),
      ),
    ),
    'colors' => array(
      'background' => 'bdcbdb',
      'foreground' => '525252',
      'link' => '913636',
      'druplicon' => 'ebc694',
    ),
    'paths' => array(
      drupal_get_path('theme', 'biurnaltheme') . '/biurnal',
    ),
    'css' => array('css/style.css'),
    'images' => array(
      'img' => array(
        'menu_sprites.png' => array(
          array('load'=>'sprites', array('images/menu-sprites.png')),
          array('canvas_from'=>'background', array('&$sprites', '$background')),
          array('copy', array('&$sprites', '&$canvas')),
          array('save', array('&$canvas')),
        ),
        array(
          'path' => 'images/menu-sprites.png',
          'alter' => array('on_background'),
        ),
        array(
          'path' => 'images/menu-sprites-bg.png',
          'alter' => array('on_background'),
        ),
        array(
          'path' => 'images/play.png',
          'alter' => array(
            array(
              'background' => array(
                'clear' => array('$link'),
              ),
              'postprocess' => array(
                'mask' => 'images/play-mask.png',
              ),
            ),
          ),
        ),
        array(
          'path' => 'images/druplicon.png',
          'alter' => array(
            array(
              'background' => array(
                'clear' => array('$druplicon'),
              ),
              'postprocess' => array(
                'mask' => 'images/druplicon-mask.png',
              ),
            ),
          ),
        ),
      ),
    ),
  );
}