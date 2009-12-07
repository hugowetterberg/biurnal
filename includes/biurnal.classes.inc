<?php
// $Id$

require_once('biurnal.hsb.inc');

class Biurnal {
  private $state = 'uninitialized';

  private static $instance;
  private $paths;
  private $images;
  private $colors;
  private $css;
  private $processes;
  private $filters;
  private $operations;

  public static function instance() {
    if (!self::$instance) {
      self::$instance = new Biurnal();
    }
    return self::$instance;
  }

  function __construct() {
    $this->paths      = array();
    $this->images     = array();
    $this->colors     = array();
    $this->css        = array();
    $this->processes  = array();
    $this->filters    = array();
    $this->operations = array();

    $this->filters['mix']              = array($this, 'mix_colors');
    $this->filters['shift_hue']        = 'biurnal_shift_hue';
    $this->filters['brightness']       = 'biurnal_set_brightness';
    $this->filters['saturation']       = 'biurnal_set_saturation';
    $this->filters['brightness_and_saturation'] = 'biurnal_set_brightness_and_saturation';
    $this->filters['match_brightness'] = 'biurnal_match_brightness';
    $this->filters['match_saturation'] = 'biurnal_match_saturation';
    $this->filters['match']            = 'biurnal_match_saturation_and_brightness';

    $this->operations['clear']          = array($this, 'clear_to_color');
    $this->operations['clear_gradient'] = array($this, 'clear_to_gradient');
    $this->operations['mask']           = array($this, 'mask');
  }

  public function init() {
    if ($this->state == 'uninitialized') {
      $this->state = 'initialized';
      $this->biurnal_path = file_directory_path() . '/biurnal';
      if (!is_dir($this->biurnal_path)) {
        mkdir($this->biurnal_path);
      }
    }
  }

  public function resolve_path($path) {
    foreach ($this->paths as $lp) {
      if (file_exists($lp . '/' . $path)) {
        return realpath($lp . '/' . $path);
      }
    }
    return FALSE;
  }

  public function read_data($data) {
    foreach ($data as $key => $values) {
      switch ($key) {
        case 'paths':
          $this->paths = array_merge($values, $this->paths);
          break;
        case 'colors':
          foreach ($values as $key => $value) {
            foreach ($value as $name => $color) {
              if($name == 'color') {
                $this->set_color($key, intval($color, 16));
              }
            }
          }
          break;
        case 'css':
          foreach ($values as $path) {
            $this->add_css($path);
          }
          break;
        case 'images':
          foreach ($values as $dir => $images) {
            foreach ($images as $image) {
              $path = $this->resolve_path($image['path'], $base_path);
              $properties = isset($image['properties'])?$image['properties']:array();
              $this->add_image($image['path'], $image['alter'], $dir, $properties);
            }
          }
          break;
        case 'processes':
          foreach ($values as $name => $process) {
            $this->add_process($name, $process);
          }
          break;
      }
    }
  }

  public function mix_colors($a, $b, $proportions = 0.5) {
    list($r1, $g1, $b1) = $a;
    list($r2, $g2, $b2) = $b;
    $r = ($r2 - $r1 != 0) ? intval($r1 + ($r2 - $r1) * $proportions): $r1;
    $g = ($g2 - $g1 != 0) ? intval($g1 + ($g2 - $g1) * $proportions): $g1;
    $b = ($b2 - $b1 != 0) ? intval($b1 + ($b2 - $b1) * $proportions): $b1;
    return array($r, $g, $b);
  }

  public function add_colors($a, $b, $proportions = 0.5) {
    list($r1, $g1, $b1) = $a;
    list($r2, $g2, $b2) = $b;
    $c = array($r1+$r2, $g1+$g2, $b1+$b2);

    return $c;
  }

  private function image_reference($reference, $obj) {
    $matches = array();
    if (preg_match('/^\$([^\/])\/(.+)$/', $reference, $matches)) {
      return $this->scheme_path . '/' . $matches[1] . '/' . $matches[2];
    }
    else {
      return $this->resolve_path($reference, $obj['properties']['base_path']);
    }
  }

  public function mask($obj, $image, $mask_image, $alpha='red') {
    $path = $this->image_reference($mask_image, $obj);
    if (file_exists($path)) {
      $mask = imagecreatefrompng($path);
      imagesavealpha($mask, TRUE);

      if ($mask) {
        $width     = min(imagesx($mask), imagesx($image));
        $height    = min(imagesy($mask), imagesy($image));
        $composite = imagecreatetruecolor($width, $height);
        imagealphablending($composite, FALSE);
        imagesavealpha($composite, TRUE);

        for ($x = 0; $x < $width; $x++) {
          for ($y = 0; $y < $height; $y++) {
            //Extract red channel from mask pixel
            $mask_rgb   = imagecolorat($mask, $x, $y);
            switch ($alpha) {
              case 'red':
                $mask_alpha = max(127 - (($mask_rgb & 0xFF0000) >> 16) / 2, 0);
                break;
              case 'green':
                $mask_alpha = max(127 - (($mask_rgb & 0x00FF00) >> 8) / 2, 0);
                break;
              case 'blue':
                $mask_alpha = max(127 - (($mask_rgb & 0x0000FF)) / 2, 0);
                break;
              default: // Alpha
                $mask_alpha = ($mask_rgb & 0x7F000000) >> 24;
            }

            //Extract color channels from image
            $rgb   = imagecolorat($image, $x, $y);
            $red   = ($rgb & 0xFF0000) >> 16;
            $green = ($rgb & 0x00FF00) >> 8;
            $blue  = ($rgb & 0x0000FF);
            $color = imagecolorallocatealpha($composite, $red, $green, $blue, $mask_alpha);
            imagesetpixel($composite, $x, $y, $color);
          }
        }

        imagedestroy($image);
        return $composite;
      }
    }
    return $image;
  }

  public function copy_preserve_alpha($obj, $image, $color=Null) {
    $width      = imagesx($image);
    $height     = imagesy($image);
    $background = imagecreatetruecolor($width, $height);

    $bg_color = -1;
    if (isset($color)) {
      $bg_color = imagecolorallocate($background, $color[0], $color[1], $color[2]);
    }
    else {
      $bg_color = imagecolorallocatealpha($background, 0, 0, 0, 0);
    }

    imagefilledrectangle($background, 0, 0, $width, $height, $bg_color);
    imagecopy($background, $image, 0, 0, 0, 0, $width, $height);

    for ($x = 0; $x < $width; $x++) {
      for ($y = 0; $y < $width; $y++) {
        $b_rgba = imagecolorat($background, $x, $y);
        $i_rgba = imagecolorat($image, $x, $y);

        $alpha = ($i_rgba & 0x7F000000) >> 24;
        $red   = ($b_rgba & 0xFF0000) >> 16;
        $green = ($b_rgba & 0x00FF00) >> 8;
        $blue  = ($b_rgba & 0x0000FF);

        $c = imagecolorallocatealpha($image, $red, $green, $blue, $alpha);
        imagesetpixel($image, $x, $y, $c);
      }
    }
    imagedestroy($background);

    return $image;
  }

  public function clear_to_color($obj, $image, $color = NULL) {
    $width  = imagesx($image);
    $height = imagesy($image);
    imagedestroy($image);
    $background = imagecreatetruecolor($width, $height);

    $bg_color = -1;
    if (isset($color)) {
      $bg_color = imagecolorallocate($background, $color[0], $color[1], $color[2]);
    }
    else {
      $bg_color = imagecolorallocatealpha($background, 0, 0, 0, 0);
    }

    imagefilledrectangle($background, 0, 0, $width, $height, $bg_color);

    return $background;
  }

  public function clear_to_gradient($obj, $image, $color_a, $color_b, $direction="vertical") {
    list($r1, $g1, $b1) = $color_a;
    list($r2, $g2, $b2) = $color_b;

    $width  = imagesx($image);
    $height = imagesy($image);
    imagedestroy($image);

    $background = imagecreatetruecolor($width, $height);

    for ($i=0; $i<$height; $i++) {
      $r = ( $r2 - $r1 != 0 ) ? intval( $r1 + ( $r2 - $r1 ) * ($i/$height) ): $r1;
      $g = ( $g2 - $g1 != 0 ) ? intval( $g1 + ( $g2 - $g1 ) * ($i/$height) ): $g1;
      $b = ( $b2 - $b1 != 0 ) ? intval( $b1 + ( $b2 - $b1 ) * ($i/$height) ): $b1;

      imagefilledrectangle($background, 0, $i, $width-1, $i, imagecolorallocate($background, $r, $g, $b));
    }

    return $background;
  }

  private function resolve_params($params) {
    $args = array();

    if (!isset($params)) {
      return $args;
    }

    foreach ((array)$params as $param) {
      if (is_string($param) && drupal_substr($param, 0, 1) == '$') {
        $args[] = $this->color(drupal_substr($param, 1));
      }
      elseif (is_string($param) && drupal_substr($param, 0, 2) == "0x") {
        $args[] = $this->intrgb(intval(drupal_substr($param, 2), 16));
      }
      elseif (is_string($param) && drupal_substr($param, 0, 2) == "##") {
        $args[] = $this->to_hsl_triplet(drupal_substr($param, 2));
      }
      elseif (is_string($param) && drupal_substr($param, 0, 1) == "#") {
        $args[] = $this->intrgb(intval(drupal_substr($param, 1), 16));
      }
      elseif (is_array($param)) {
        foreach ($param as $filter => $fparams) {
          $args[] = $this->run_filter($filter, $fparams);
          break;
        }
      }
      else {
        $args[] = $param;
      }
    }
    return $args;
  }

  private function run_filter($name, $params) {
    if (!isset($this->filters[$name])) {
      return NULL;
    }
    $args = $this->resolve_params($params);
    $res  = call_user_func_array($this->filters[$name], $args);
    return $res;
  }

  private function scheme_identifier() {
    if (module_exists('pathauto') && function_exists('pathauto_cleanstring')) {
      return pathauto_cleanstring(mb_convert_case($this->scheme->title, MB_CASE_LOWER));
    }
    else {
      return $this->scheme->id;
    }
  }

  public function get_theme_data($theme) {
    $data = array();
    $data_file = NULL;
    if ($this->theme_is_biurnal($theme, $data_file)) {
      require_once($data_file);
      if (function_exists($theme . '_biurnal_data')) {
        $data = call_user_func($theme . '_biurnal_data');
        if (!isset($data['paths'])) {
          $data['paths'][] = drupal_get_path('theme', $theme);
        }
        drupal_alter('biurnal_theme_data', $data, $theme);
      }
    }
    return $data;
  }

  public function bootstrap() {
    global $theme;

    $this->init();

    if ($this->state == 'initialized') {
      $this->state = 'bootstrapped';

      $theme_key = variable_get('theme_default', '');
      if (!empty($theme)) {
        $theme_key = $theme;
      }

      // Get biurnal data from modules
      $modules = module_implements('biurnal_data');
      foreach ($modules as $module) {
        $data = call_user_func($module . '_biurnal_data');
        if (!isset($data['paths'])) {
          $data['paths'][] = drupal_get_path('module', $module);
        }
        $this->read_data($data);
      }

      // Get biurnal data from the theme and it's parent themes
      $this->biurnal_theme_path = drupal_get_path('theme', $theme_key);
      $themes = list_themes(); 
      $current_theme = $themes[$theme_key];

      // Build a theme stack of the theme and it's parent themes
      $theme_stack = array();
      while ($current_theme) {
        if ($this->theme_is_biurnal($current_theme->name)) {
          $theme_stack[] = $current_theme;
        }
        $current_theme = empty($current_theme->base_theme) ? NULL : $themes[$current_theme->base_theme];
      }

      // Load data for the themes in the stack
      while ($t = array_pop($theme_stack)) {
        $this->read_data($this->get_theme_data($t->name));
      }
    }
  }

  public function preprocess(&$vars) {
    global $theme;
    $this->bootstrap();

    if ($this->state == 'bootstrapped') {
      $this->state = 'processed';
      $theme_key = variable_get('theme_default', '');
      if (!empty($theme)) {
        $theme_key = $theme;
      }

      //Get the active biurnal configuration
      $this->scheme = $this->get_scheme();

      //Create a mock-scheme with the default values so that we never get a blank page
      if (!isset($this->scheme) || !$this->scheme) {
        $this->scheme = (object) array('id' => 0, 'title' => 'default', 'changed' => 0);
      }
      else { //Read the colors from the biurnal configuration
        if (isset($this->scheme->configuration)) {
          $palette = $this->scheme->configuration;
          if (is_array($palette)) {
            foreach ($palette as $name => $color) {
              $this->update_color($name, intval(drupal_substr($color, 1), 16));
            }
          }
        }
      }

      //Check if the needs to be created or recreated
      $this->scheme_path = sprintf('%s/%s/%s', $this->biurnal_path, $theme_key, $this->scheme_identifier());
      if (!is_dir($this->scheme_path) || $this->scheme->changed > filemtime($this->scheme_path . '/touch.txt')) {
        //Delete old files
        if (is_dir($this->scheme_path)) {
          _biurnal_rm_recursive($this->scheme_path);
        }

        //Create new scheme files
        mkdir($this->scheme_path, 0777, TRUE);
        file_put_contents($this->scheme_path . '/touch.txt', format_date(time()));
        $this->scheme_path .= '/' . $this->scheme->changed;
        mkdir($this->scheme_path, 0777, TRUE);
        $this->process_scheme_resources();
      }
      else {
        $this->scheme_path .= '/' . $this->scheme->changed;
      }
    }

    foreach ($this->colors as $name => $color) {
      $vars['colors'][$name] = $this->color_to_hex($color);
      $vars['colors'][$name . '_triplet'] = $color;
    }

    foreach ($this->css as $css) {
      $name = basename($css);
      $vars['biurnal'] .= '<link rel="stylesheet" href="/' . $this->scheme_path . '/' . $name . '" type="text/css" />' . "\n";
      $vars['biurnal'] .= '<!--[if lte IE 7]><link rel="stylesheet" href="/' . $this->scheme_path . '/ie.' . $name . '" type="text/css" /><![endif]-->' . "\n";
    }
  }

  private function process_scheme_resources() {
    //Create new images
    foreach ($this->images as $dir_name => $images) {
      $dir = $this->scheme_path . '/' . $dir_name;
      if (!is_dir($dir)) {
        mkdir($dir);
      }

      foreach ($images as $image) {
        $path = $this->resolve_path($image['path']);
        if ($path) {
          list($width, $height) = getimagesize($path);
          $img = imagecreatefrompng($path);

          //Transparent background
          $back = imagecreatetruecolor($width, $height);
          imagesavealpha($back, TRUE);
          $trans_colour = imagecolorallocatealpha($back, 0, 0, 0, 127);
          imagefill($back, 0, 0, $trans_colour);

          $processes = NULL;
          if (isset($image['alter'])) {
            $processes = array();
            foreach ($image['alter'] as $p) {
              if (is_array($p)) {
                $processes[] = $p;
              }
              elseif (is_string($p) && isset($this->processes[$p])) {
                $processes[] = $this->processes[$p];
              }
            }

            //Run foreground and background processes
            foreach ($processes as $process) {
              if (isset($process['background'])) {
                $back = $this->apply_operations($image, $back, $process['background']);
              }
              if (isset($process['foreground'])) {
                $img = $this->apply_operations($image, $img, $process['foreground']);
              }
            }
          }

          imagecopy($back, $img, 0, 0, 0, 0, $width, $height);
          imagedestroy($img);

          //Run postpreocess operations
          if (isset($processes)) {
            foreach ($processes as $process) {
              if (isset($process['postprocess'])) {
                $back = $this->apply_operations($image, $back, $process['postprocess']);
              }
            }
          }

          $file = $dir . '/' . basename($image['path']);
          if (file_exists($file)) {
            unlink($file);
          }

          imagepng($back, $file);
          imagedestroy($back);
        }
      }
    }

    //Process css files
    foreach ($this->css as $css) {
      $name = basename($css);
      $path = $this->resolve_path($css);
      if ($path) {
        $content = file_get_contents($path);
        $content = preg_replace_callback('/\$(\{.*\})\$/', array($this, 'filter_callback'), $content);
        $content = preg_replace_callback('/\$([_a-z]+)/', array($this, 'color_callback'), $content);
        file_put_contents($this->scheme_path . '/ie.' . $name, $content);

        $content = preg_replace_callback('/url\([\'"]?([^\)\'"]+)[\'"]?\)/', array($this, 'data_url_callback'), $content);
        file_put_contents($this->scheme_path . '/' . $name, $content);
      }
    }
  }

  private function data_url_callback($matches) {
    $path = $this->scheme_path . '/' . trim($matches[1]);
    if (file_exists($path) && filesize($path) < 800) {
      return 'url(data:image/png;base64,' . base64_encode(file_get_contents($path)) . ')';
    }
    return $matches[0];
  }

  private function filter_callback($matches) {
    $filter_def = json_decode($matches[1], TRUE);
    if ($filter_def) {
      foreach ($filter_def as $filter => $fparams) {
        $val = $this->run_filter($filter, $fparams);
        if (is_array($val)) {
          return $this->color_to_hex($val);
        }
        else {
          return $val;
        }
      }
    }
  }

  private function color_callback($matches) {
    return $this->get_hex_color($matches[1]);
  }

  private function apply_operations($obj, $img, $operations) {
    foreach ($operations as $operation => $params) {
      if (isset($this->operations[$operation])) {
        $args = $this->resolve_params($params);
        array_unshift($args, $obj, $img);
        $img = call_user_func_array($this->operations[$operation], $args);
      }
    }
    return $img;
  }

  public function intrgb($int) {
    return array(0xFF & ($int >> 0x10), 0xFF & ($int >> 0x8), 0xFF & $int);
  }

  public function get_colors() {
    return $this->colors;
  }

  public function get_colors_hex() {
    $colors = array();

    foreach ($this->colors as $name => $rgb) {
      $colors[$name] = $this->color_to_hex($rgb);
    }

    return $colors;
  }

  public function theme_is_biurnal($name, &$biurnal_file=NULL) {
    $biurnal_file = drupal_get_path('theme', $name) . '/' . $name . '.biurnal.inc';
    return file_exists($biurnal_file);
  }

  public function set_scheme($nid) {
    variable_set('biurnal_scheme', $nid);
  }

  public function stop_preview() {
    unset($_SESSION['biurnal_preview']);
  }

  public function preview_scheme($nid = NULL) {
    if (isset($nid)) {
      $_SESSION['biurnal_preview'] = $nid;
      return TRUE;
    }
    else {
      if (isset($_SESSION['biurnal_preview'])) {
        $nid = $_SESSION['biurnal_preview'];
      }

      if (!empty($nid)) {
        return $nid;
      }
      else {
        return FALSE;
      }
    }
  }

  public function get_scheme() {
    $configuration = FALSE;
    $id = $this->preview_scheme();
    if (!$id) {
      $id = variable_get('biurnal_active_configuration', 0);
    }
    if ($id) {
      $configuration = biurnal_configuration_load($id);
    }
    return $configuration;
  }

  public function color($name = 'main') {
    if (isset($this->colors) && isset($this->colors[$name])) {
      return $this->colors[$name];
    }
    return array(0, 0, 0);
  }

  public function add_image($path, $alter = NULL, $dir = 'img', $properties = array()) {
    $dir = basename($dir);
    $this->images[$dir][] = array('path' => $path, 'alter' => $alter, 'properties' => $properties);
  }

  public function add_process($name, $process) {
    $this->processes[$name] = $process;
  }

  public function set_color($name, $color) {
    if (is_int($color))
      $color = $this->intrgb($color);
    $this->colors[$name]=$color;
  }

  public function update_color($name, $color) {
    if (isset($this->colors[$name]))
      $this->set_color($name, $color);
  }

  public function get_hex_color($name) {
    $color = $this->color($name);
    return sprintf('#%02x%02x%02x', $color[0], $color[1], $color[2]);
  }

  public function color_to_hex($color) {
    return sprintf('#%02x%02x%02x', $color[0], $color[1], $color[2]);
  }

  public function add_css($path) {
    $this->css[] = $path;
  }

  private function to_hsl_triplet($str) {
    $args = split(',', $str);
    return array(intval($args[0]), intval($args[1]), intval($args[2]));
  }

  /**
   * Convert a HSL triplet into RGB
   */
  private function hsl2rgb($hsl) {
    $h  = $hsl[0];
    $s  = $hsl[1];
    $l  = $hsl[2];
    $m2 = ($l <= 0.5) ? $l * ($s + 1) : $l + $s - $l*$s;
    $m1 = $l * 2 - $m2;
    return array(
      $this->hue2rgb($m1, $m2, $h + 0.33333),
      $this->hue2rgb($m1, $m2, $h),
      $this->hue2rgb($m1, $m2, $h - 0.33333),
    );
  }

  /**
   * Helper function for hsl2rgb().
   */
  private function hue2rgb($m1, $m2, $h) {
    $h = ($h < 0) ? $h + 1 : (($h > 1) ? $h - 1 : $h);
    if ($h * 6 < 1) return $m1 + ($m2 - $m1) * $h * 6;
    if ($h * 2 < 1) return $m2;
    if ($h * 3 < 2) return $m1 + ($m2 - $m1) * (0.66666 - $h) * 6;
    return $m1;
  }

  /**
   * Convert an RGB triplet to HSL.
   */
  private function rgb2hsl($rgb) {
    $r = $rgb[0];
    $g = $rgb[1];
    $b = $rgb[2];
    $min = min($r, min($g, $b));
    $max = max($r, max($g, $b));
    $delta = $max - $min;
    $l = ($min + $max) / 2;
    $s = 0;
    if ($l > 0 && $l < 1) {
      $s = $delta / ($l < 0.5 ? (2 * $l) : (2 - 2 * $l));
    }
    $h = 0;
    if ($delta > 0) {
      if ($max == $r && $max != $g) $h += ($g - $b) / $delta;
      if ($max == $g && $max != $b) $h += (2 + ($b - $r) / $delta);
      if ($max == $b && $max != $r) $h += (4 + ($r - $g) / $delta);
      $h /= 6;
    }
    return array($h, $s, $l);
  }
}