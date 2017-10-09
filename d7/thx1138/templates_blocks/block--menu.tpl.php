<?php
// menu
// http://api.drupal.org/api/drupal/modules--block--block.tpl.php/7
// <h2 class="element-invisible">...</h2>
if ($classes) {
  $classes = ' class="'. $classes . ' "';
}
?>


<input type="checkbox" id="block-<?php print $block->module .'-'. $block->delta ?>-toggle" class="toggle-check" name="toggler" />
<label for="block-<?php print $block->module .'-'. $block->delta ?>-toggle" class="toggle" onclick>
  <span class="icon" aria-hidden="true"></span>
  <span class="icon-text"><?php print $block->subject ?></span>
</label>



<div <?php print $id_block . $classes .  $attributes; ?> role="navigation">
  <?php print $mothership_poorthemers_helper;  ?>

  <?php print render($title_prefix); ?>
  <?php if ($block->subject): ?>
    <h2<?php print $title_attributes; ?>><?php print $block->subject ?></h2>
  <?php endif;?>
  <?php print render($title_suffix); ?>

  <?php print $content ?>


  <?php if (!theme_get_setting('mothership_classes_block_contentdiv') AND $block->module == "block"): ?>
  <div <?php print $content_attributes; ?>>
  <?php endif ?>




  <?php if (!theme_get_setting('mothership_classes_block_contentdiv') AND $block->module == "block"): ?>
  </div>
  <?php endif ?>

</div>
