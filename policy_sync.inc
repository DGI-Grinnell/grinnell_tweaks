<?php 

function grinnell_tweaks_sync_policy($pid, &$context) {

  module_load_include('inc','fedora_repository','api/fedora_item');
  module_load_include('inc','fedora_repository','api/fedora_collection');
  module_load_include('inc','islandora_xacml_editor','fedora_item_plus');
  
  if (!isset($context['sandbox']['progress'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['current_item'] = 0;
    $context['sandbox']['max'] = 1000; // fix this
  }
  
  $fi = new Fedora_Item_Plus($pid);
  $xml = $fi->get_datastream_dissemination('POLICY');

  $doc = new DomDocument();
  $doc->loadXML($xml);

  $rules = $doc->getElementsByTagName('Rule');
  $ISLANDORA_RELSEXT = 'http://islandora.ca/ontology/relsext#';
  foreach($rules as $rule) {
    $ruleid = $rule->getAttribute('RuleId');
    if ($ruleid == 'deny-access-functions') {
      $sads = $rule->getElementsByTagName('SubjectAttributeDesignator');
      foreach($sads as $sad) {
        $attrid = $sad->getAttribute('AttributeId');
        $relationship = ($attrid == 'fedoraRole')? 'isViewableByRole': 'isViewableByUser';
        $dad = $sad->parentNode;
        $subjects = $dad->getElementsByTagName('AttributeValue');
        $fi->purge_relationships($relationship,$ISLANDORA_RELSEXT);
        foreach($subjects as $subject) {
          $params = array( 
            'pid' => $pid,
            'relationship' => $ISLANDORA_RELSEXT.$relationship,
            'object' => $subject->value,
            'isLiteral' => TRUE,
            'datatype' => '',
          );
          $fi->soap_call('addRelationship', $params);
        }
      }
    }
  }
  // Update our progress information.
  $context['sandbox']['progress']++;
  $context['sandbox']['current_item'] = $pid;
  $context['message'] = t('Now processing %pid', array('%pid' => $pid));
}

/**
 * Batch 'finished' callback
 */
function grinnell_tweaks_policy_sync_finished($success, $results, $operations) {
  if ($success) {
    // Here we do something meaningful with the results.
    $message = count($results) .' processed.';
    $message .= theme('item_list', $results);
  }
  else {
    // An error occurred.
    // $operations contains the operations that remained unprocessed.
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array('%error_operation' => $error_operation[0], '@arguments' => print_r($error_operation[1], TRUE)));
  }
  drupal_set_message($message);
}

