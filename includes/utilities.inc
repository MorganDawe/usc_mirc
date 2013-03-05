<?php

function usc_mirc_get_image_markup($pid, $alt = '', $title = '', $imagecache_preset = NULL) {
  $object = islandora_object_load($pid);
  $imagecache_preset = variable_get('usc_mirc_imagecache_thumbnail_preset', 'usc_mirc_thumbnail');

  $dsid = '';
  if ($object['IMG']) {
    $dsid = 'IMG';
  }
  else {
    $dsid = 'TN';
  }
  $image = "islandora/object/$pid/datastream/$dsid/view";

  if (module_exists('imagecache_external') && is_callable('theme_imagecache_external')) {
    return theme('imagecache_external', array('path' => url($image, array('absolute' => TRUE)), 'style_name' => $imagecache_preset, 'alt' => 'Collection image'));
  }
  else {
    return theme_image(array('path' => $image, 'alt' => $alt, 'title' => $title, 'attributes' => array()));
  }
}

function usc_mirc_get_collection_set($object, $page = 0, $number = 20, $mezzanines = TRUE) {
  $offset = $page * $number;
  $collection_relation = variable_get('usc_mirc_collection_relationship', 'isMemberOfCollection');
  $derivative_relation = variable_get('usc_mirc_relationship', 'isDerivativeOf');
  $vro_cmodel = variable_get('usc_mirc_cmodel_vro', 'usc:test-vro');
  $mezzanine_cmodel = variable_get('usc_mirc_cmodel_mezzanine', 'usc:test-mezzanine');

  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  PREFIX dc: <http://purl.org/dc/elements/1.1/>
  SELECT ?vro " . ($mezzanines ? " ?mezzanine ?mezzanine_title" : "?vro_title") . "
  WHERE {
  ?vro fre:$collection_relation <info:fedora/$object->id> ;
       fm:hasModel <info:fedora/$vro_cmodel> ; " .
       ((!$mezzanines) ? " dc:title ?vro_title ; " : '') . "
       fm:state fm:Active . " . ($mezzanines ? "
  OPTIONAL{
  ?mezzanine fre:$derivative_relation ?vro ;
             fm:hasModel <info:fedora/$mezzanine_cmodel> ;
             fm:state fm:Active ;
             dc:title ?mezzanine_title .
  }" : '') . "
  }
  ORDER BY " . ($mezzanines? "?mezzanine_title" : "?vro_title") . "
  LIMIT $number
  OFFSET $offset";

  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
    'page_size' => $page,
    'page_number' => $number
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  return $results;
}

function usc_mirc_get_number_of_children($object, $mezzanines = TRUE) {
  $collection_relation = variable_get('usc_mirc_collection_relationship', 'isMemberOfCollection');
  $derivative_relation = variable_get('usc_mirc_relationship', 'isDerivativeOf');
  $vro_cmodel = variable_get('usc_mirc_cmodel_vro', 'usc:test-vro');
  $mezzanine_cmodel = variable_get('usc_mirc_cmodel_mezzanine', 'usc:test-mezzanine');

  $query = "
  PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  PREFIX dc: <http://purl.org/dc/elements/1.1/>
  SELECT ?vro
  WHERE {
    ?vro fre:$collection_relation <info:fedora/$object->id> ;
         fm:hasModel <info:fedora/$vro_cmodel> ;
         fm:state fm:Active . " . ($mezzanines ? "
    ?mezzanine fre:$derivative_relation ?vro ;
               fm:hasModel <info:fedora/$mezzanine_cmodel> ;
               fm:state fm:Active . " : '') . "
  }";
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  return count($results);
}

function usc_mirc_get_mezzanine_parent($object) {
  $vro_cmodel = 'usc:test-vro';
  $def_rel = variable_get('usc_mirc_relationship', 'isDerivativeOf');
  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  SELECT ?object
  WHERE {
    ?subject fre:$def_rel ?object ;
             fm:state fm:Active .
    ?object fm:state fm:Active ;
            fm:hasModel <info:fedora/$vro_cmodel> .
    FILTER(sameTerm(?subject, <info:fedora/$object->id>)) .
  }";

  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);

  if(count($results) > 0) {
    return $results[0]['object']['value'];
  }
  else {
    return FALSE;
  }
}

function usc_mirc_mezzanine_player_markup($videos) {
  global $base_url;
  $mod_path = drupal_get_path('module', 'usc_mirc');
    drupal_add_js("$mod_path/lib/flowplayer/flowplayer-3.2.8.min.js", array('group' => JS_LIBRARY));

    $player_class = 'usc-flowplayer';
    drupal_add_js(<<<EOJS
jQuery(function() {
  jQuery('.$player_class').flowplayer("$base_url/$mod_path/lib/flowplayer/flowplayer.commercial-3.2.8.swf", {

    //Product key (for sc.edu)
    key: "#\$60fe88f4c446bc34a1c",

    clip: {
      provider: 'lighttpd', // configure clip to use "lighthttpd" plugin for providing video data
      scaling: 'fit', //Maintain aspect ratio when scaling
    },
    canvas: { //Use a full-black background
      background: '#000000',
      backgroundGradient: 'none',
    },

    // streaming plugins are configured normally under the plugins node
    plugins: {
      lighttpd: {
        url: 'flowplayer.pseudostreaming-3.2.8.swf'
      }
    }
  })
});
EOJS
, array('type' => 'inline'));

    if (count($videos) > 0) {
      $output = array(
        '#markup' => l(theme_image(array("path" => "$mod_path/images/playbuttonblack.jpg", 'alt' => 'Video placeholder', 'attributes' => array())), $videos[0],
         array(
          'html' => TRUE,
          'attributes' => array(
            'class' => array($player_class),
          ),
         )),
      );
    }
    else {
      $output = array(
        '#markup' => theme_image(array("path" => "$mod_path/images/NoVideoAvailable.jpg")),
      );
    }
    return drupal_render($output);
}

function usc_mirc_get_derivatives($object, $subject = NULL) {
  if ($subject === NULL) {
    $def_rel = variable_get('usc_mirc_relationship', 'isDerivativeOf');
    $subject = "?subject fre:$def_rel <info:fedora/$object->id>";
  }
  //perform query
  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  SELECT ?subject ?label
  WHERE {
  $subject ;
         fm:label ?label ;
         fm:state fm:Active .
  }";
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  return $results;
}

function usc_mirc_build_breadcrumb($object) {
  $vro_cmodel = variable_get('usc_mirc_cmodel_vro', 'usc:test-vro');
  $mezz_cmodel = variable_get('usc_mirc_cmodel_mezzanine', 'usc:test-mezzanine');
  $pid = $object->id;
  $query = "PREFIX fre: <info:fedora/fedora-system:def/relations-external#>
  PREFIX fm: <info:fedora/fedora-system:def/model#>
  SELECT ?parent
  WHERE {
    {
      ?this fre:isDerivativeOf ?vro ;
            fm:hasModel <info:fedora/$mezz_cmodel> ;
            fm:state fm:Active .
      ?vro fre:isMemberOfCollection ?parent ;
           fm:hasModel <info:fedora/$vro_cmodel> ;
           fm:state fm:Active .
    }
    UNION
    {
      ?this fre:isMemberOfCollection ?parent ;
            fm:hasModel ?model ;
            fm:state fm:Active .
      FILTER(!sameterm(?model, <info:fedora/$vro_cmodel>)) .
    }
    FILTER(sameterm(?this, <info:fedora/$pid>))
  }";
  $query_array = array(
    'query' => $query,
    'type' => 'sparql',
  );
  $results = $object->repository->ri->query($query_array['query'], $query_array['type']);
  $parent_object = islandora_object_load($results[0]['parent']['value']);
  $title = $parent_object->label;
  return l($title, "islandora/object/$parent_object->id");
}