<?php
use Drupal\Core\Database\Database;
use Drupal\Core\Database\Query\Condition;
use Drupal\image\Entity\ImageStyle;
use Drupal\file\Entity\File;

/**
 * Implements hook_drush_command().
 */

function migrate_jcl_drush_command() {
  $items = array(
    'seed-image-derivatives' =>  array(
      'category' => 'Custom Commands',
      'description' => "Create image derivatives",
      'options' => array(
        'include' => array(
            'description' => 'A comma separated list of all image styles needing generation, leaving this blank will use all styles',
            'example_value' => 'small,medium,thumbnail',
        ),
        'exclude' => array(
            'description' => 'A comma separated list of all image styles to be excluded. Leaving this blank will use all styles from the "styles" parameter.',
            'example_value' => 'large',
        ),
        'dir' => array(
            'description' => 'Set to the file path (relative to your public files directory) under which all files will have an image derivative created. Alternatively, set to "public" to just have your public files directory looked into with no recursion. With no option set, the default will run through every single file.',
            'example_value' => 'field/image',
        ),
        'purge' => array(
            'description' => 'Flag to delete files if they already exist. This can be useful for replacing files that have the same name but updated content.',
        ),
      ),
      'examples' => array(
        'drush seed-image-derivatives' => 'Creates image derivatives for every single image style for every file within the public file system.',
        'drush seed-image-derivatives --include=large,medium --dir=field' => 'Creates large and medium image derivatives for every single image stored underneath the "field" directory in the public file directory.',
        'drush seed-image-derivatives --exclude=thumbnail --dir=public' => 'Creates image derivatives for every single image style except "thumbnail" for every file within the public file system directory but not in subdirectories.',
        'drush seed-image-derivatives --include=thumbnail --dir=public --purge' => 'Creates image derivatives for the "thumbnail" style only and overwriting images if they exist, for every file within the public file system directory but not in subdirectories.',
        'drush sid' => 'Creates image derivatives for every single image style for every file within the public file system.',
        'drush sid --include=large,medium --dir=field' => 'Creates large and medium image derivatives for every single image stored underneath the "field" directory in the public file directory.',
        'drush sid --exclude=thumbnail --dir=public' => 'Creates image derivatives for every single image style except "thumbnail" for every file within the public file system directory but not in subdirectories.',
        'drush sid --include=thumbnail --dir=public --purge' => 'Creates image derivatives for the "thumbnail" style only and overwriting images if they exist, for every file within the public file system directory but not in subdirectories.',
      ),
      'core' => ['8'],
      'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
      'callback' => 'seed_image_derivatives_drush_action',
      'drupal dependencies' => ['migrate_jcl'], # the containing module
      'aliases' => ['sid'],
    ),

    'migrate-prune-configs' => array(
      'description' => dt('Prune migrate configs.'),
      'aliases' => ['mpc'],
      'examples' => array(
        'drush migrate-prune-configs' => 'Removes migrate configurations.',
        'drush mpc' => 'Removes migrate configurations.',
      ),
      'callback' => 'drush_migrate_jcl_migrate_prune_configs',
    ),

    'moduler-config-reset' => array(
      'description' => dt('Resets default config for a given module.'),
      'aliases' => ['mcr'],
      'examples' => array(
        'drush migrate-config-reset' => 'Resets migrate configurations.',
        'drush mcr' => 'Removes migrate configurations.',
      ),
      'callback' => 'drush_migrate_jcl_config_reset',
    ),

    'migrate-re-register-migrations' => array(
      'description' => dt('Re-register migrations and groups.'),
      'aliases' => ['mrm'],
      'examples' => array(
        'drush migrate-re-register-migrations' => 'Re-register migrations and groups.',
        'drush mrm' => 'Re-register migrations and groups.',
      ),
      'callback' => 'drush_migrate_jcl_re_register_migrations',
    ),
  );
  return $items;
}

function seed_image_derivatives_drush_action() {
  $purge = drush_get_option('purge', false);
  $extensions = array('jpeg', 'jpg', 'gif', 'png');
  $mimetypes = array('image/jpeg', 'image/jpg', 'image/gif', 'image/png');

  $dir = rtrim(drush_get_option('dir'), '/');
  if ($dir == 'public') {
    $file_pattern = "[^\/]*"; // Finds anything that does not contain "/", should be fine.
  }
  else {
    $file_pattern = $dir ? $dir . ".+" : ".+";
  }

  $regex = "^public:\/\/(" . $file_pattern . ")\.(" . implode($extensions, '|') . ")$";

  // Query the database for files that match this pattern.
  $query = Database::getConnection()
    ->select('file_managed', 'f')
  ;
  $andGroup = $query->andConditionGroup()
    ->condition('filemime', $mimetypes , 'IN')
    ->condition('uri', $regex, 'REGEXP')
  ;
  $query->condition($andGroup);

  $total_count = $query
    ->countQuery()
    ->execute()
    ->fetchField()
  ;

  drush_print(\Drupal::translation()->formatPlural($total_count,
    '1 entry is to have its image derivatives created.',
    '@count entries are to have their image derivatives created.')
  );

  // Select the files to have derivatives created..
  $files = $query->fields('f', array('fid', 'filename', 'uri'))
    ->execute()
    ->fetchAll()
  ;

  $excludes = explode(",", drush_get_option('exclude'));
  $includes = explode(",", drush_get_option('include'));
  $imageStyles = ImageStyle::loadMultiple();

  /**
   * @var string $name Name of Style
   * @var ImageStyle $style ImageStyle Object
   */
  foreach ($imageStyles as $name => $style) {
    if (!empty($excludes) && in_array($name, $excludes)) {
      drush_print('Excluding ' . $name);
      continue;
    }
    if (empty($includes[0]) || in_array($name, $includes)) {
      drush_print("Processing Style $name");
      $count = 1;
      foreach ($files as $file) {
        $destination = $style->buildUri($file->uri);
        if ($purge) {
            if (file_exists($destination)) {
                unlink($destination);
            }
        }
        if (!file_exists($destination)) {
            $style->createDerivative($file->uri, $destination);
        }
        __show_progress($count, count($files), $name);
        $count++;
      }
      drush_print("Style $name Processed");
    }
  }
}

function __show_progress($count, $total, $name) {
  if (!isset($display)) static $display = [];
  if (!array_key_exists($name, $display)) {
    $display[$name] = range(25, 100, 25); # integer steps to display <= 100
  }
  $progress = ($count / $total) * 100;
  if (count($display[$name]) and $progress >= $display[$name][0]) {
    drush_print("Style $name progress " . $display[$name][0] . "%");
    array_shift($display[$name]);
  }
}

/**
 * Removes configs for a given module.
 */
function drush_migrate_jcl_migrate_prune_configs() {
  //Confirmation will be Y/N when use type “y” the condition will be executed if not, it will not.
  if (drush_confirm(dt("Are you sure you want to remove migrate configs?"))) {
    drush_print("Removing.....");
    if (_migrate_jcl_remove_configs('migrate_jcl')) {
      drush_print("Removed successfully");
      return TRUE;
    }
  }
}

/**
 * Installs configs for a given module.
 * @param  string 'migrate_jcl' module name.
 */
function drush_migrate_jcl_config_reset() {
  if (!in_array('migrate_jcl', array_keys(\Drupal::moduleHandler()->getModuleList()))) {
    return drush_set_error(dt('Module @module_name not found.', ['@module_name' => 'migrate_jcl']));
  }
  // Initiate config installer for given module.
  if (_migrate_jcl_install_config('migrate_jcl')) {
    drush_print('Configuration was successfully reset.');
    return TRUE;
  }
}

/**
 * Drush command callback for re-registering migration configs.
 * @param  string 'migrate_jcl' module name.
 */
function drush_migrate_jcl_re_register_migrations() {
  // Check input.
  if (!in_array('migrate_jcl', array_keys(\Drupal::moduleHandler()->getModuleList()))) {
	  return drush_set_error(dt('Module @module_name not found.', ['@module_name' => 'migrate_jcl']));
  }
  // Confirm re-registering.
  if (drush_confirm(dt("Are you sure you want to re-register migrate configs?"))) {
  	drush_print("Removing.....");
  	if (_migrate_jcl_remove_configs('migrate_jcl')) {
    	drush_print("Removed successfully");
  }
	else {
  	drush_print("There was a problem removing old configs");
	}

	if (_migrate_jcl_install_config()) {
  	drush_print("Configuration was successfully reset.");
	}
	else {
  	drush_print("There was a problem resetting configuration for migrate_jcl.");
	}

	return TRUE;
  }
}

/**
 * Helper module to remove configs for a given module.
 * @param  string 'migrate_jcl' module name.
 * @return bool            	TRUE if the files exist.
 */
function _migrate_jcl_remove_configs() {
  // Get the path for the module we want to remove configs for.
  $module_path = DRUPAL_ROOT. base_path() . drupal_get_path('module', 'migrate_jcl') . '/config/install';

  // Get all of the migration config names.
  $configs = array();
  $files = file_scan_directory($module_path, '/migrate_plus/');

  if ($files) {
  	foreach ($files as $file) {
    	$configs[] = $file->name;
  	}

  	// Delete each config using configFactory.
  	foreach ($configs as $config_name) {
      drush_print("..." . $config_name);
  	  \Drupal::configFactory()->getEditable($config_name)->delete(); }
  	  return TRUE;
    }
  return FALSE;
}

/**
 * Helper module to install configs for a given module.
 * @param  string 'migrate_jcl' module name.
 * @return bool            	TRUE if no error from installDefaultConfig.
 */
function _migrate_jcl_install_config() {
  // Initiate config installer for given module.
  \Drupal::service('config.installer')->installDefaultConfig('module', 'migrate_jcl');

  return TRUE;
}
