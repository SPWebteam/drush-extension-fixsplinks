<?php
/**
 * @file
 * Drush plugin to globally fix links in ALL text fields.
 *
 * Eg: replace http://www.weert.sp.nl with https://weert.sp.nl.
 */

/**
 * Implements hook_drush_command().
 */
function fixsplinks_drush_command() {
  $items['fix-sp-links'] = array(
    'aliases' => array('fsl'),
    'callback' => 'drush_fixsplinks_replace',
    'description' => dt('Fix links in text fields in all content.'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_FULL,
  );

  return $items;
}

/**
 * Implements hook_hook_drush_command_validate().
 */
function drush_fixsplinks_replace_validate() {
  $args = func_get_args();
  if (empty($args[1])) {
    drush_set_error('NO_TEXT', dt('You must specify text to search & replace.'));
  }
}

/**
 * Command callback.
 *
 * This callback checks the version of Drupal it's operating on and runs an
 * appropriate function to do the actual work. If an unsupported version is
 * detected, it will exit.
 */
function drush_fixsplinks_replace() {
  $search = 'https?://(www\.)?([A-Za-z0-9_\-]+)\.sp\.nl';
  $replace = 'https://\\2.sp.nl';

  drush_print(dt('Going fix links in all text fields in all bundles and all custom blocks on @site', array('@site' => variable_get('site_name', 'Drupal'))));

  $continue = drush_confirm(dt('There is no undo. Have you just created a database backup?'));
  if (empty($continue)) {
    drush_log(dt('Aborting'), 'warning');
    return 0;
  }

  $continue = drush_confirm(dt('There is no undo. Are you sure this is what you want to do?'));
  if (empty($continue)) {
    drush_log(dt('Aborting'), 'warning');
    return 0;
  }

  switch (drush_drupal_major_version()) {
    case 7:
      return _drush_fixsplinks_replace_d7($search, $replace);

    default:
      drush_log(dt('This version of Drupal is unsupported.'), 'error');
      return 0;
  }
}

/**
 * Does the search and replace for Drupal 7 sites.
 */
function _drush_fixsplinks_replace_d7($search, $replace) {
  // Grab all defined fields, then cycle through them and run update queries.
  $fields = field_info_fields();
  foreach ($fields as $field) {
    // Skip fields that aren't provided by the text module.
    if ($field['module'] != 'text') {
      continue;
    }

    // Use (private, naughty) API calls to grab the table and column names.
    $data_table = _field_sql_storage_tablename($field);
    $revision_table = _field_sql_storage_revision_tablename($field);
    $column = _field_sql_storage_columnname($field['field_name'], 'value');

    $bundles = array();
    foreach ($field['bundles'] as $entity => $bundle) {
      $bundles += array_values($bundle);
    }

    drush_log(dt('Processing field @name (@bundles)', array('@name' => $field['field_name'], '@bundles' => implode(', ', $bundles))), 'ok');

    // Update data in FIELD_LOAD_CURRENT.
    $updated = db_update($data_table)
      ->expression($column, "REGEXP_REPLACE(${column}, :search, :replace)", array(':search' => $search, ':replace' => $replace))
      ->execute();
    drush_log(dt('  Updated @bundle.', array('@bundle' => format_plural($updated, '1 bundle', '@count bundles'))), 'ok');

    // Update data in FIELD_LOAD_REVISION.
    $updated = db_update($revision_table)
      ->expression($column, "REGEXP_REPLACE(${column}, :search, :replace)", array(':search' => $search, ':replace' => $replace))
      ->execute();
    drush_log(dt('  Updated @revision.', array('@revision' => format_plural($updated, '1 bundle revision', '@count bundle revisions'))), 'ok');
  }

  // Do the blocks too.
  $updated = db_update('block_custom')
    ->expression('body', "REGEXP_REPLACE(body, :old, :new)", array(':old' => $search, ':new' => $replace))
    ->execute();
  drush_log(dt('  Updated @block.', array('@block' => format_plural($updated, '1 custom block', '@count custom blocks'))), 'ok');

  return 1;
}
