<?php

require dirname(__FILE__) . DIRECTORY_SEPARATOR . 'config' . DIRECTORY_SEPARATOR . 'setup.inc.php';

$method = $_REQUEST['m'];
if (isset($method) == false) {
    json_err_return(PARAMETER_ERROR);
    exit();
}

$method = (int)$method;

switch($method) {
    case GET_HINTS_CMD_ID:
        $keywords = $_REQUEST['wd'];
        if (isset($keywords) == false || $keywords == '') {
            json_err_return(PARAMETER_ERROR);
            break;
        }

        $trimmed_keywords = trim($keywords);
        $ret = get_search_hints($trimmed_keywords);
        if ($ret === false) {
            json_err_return(SEARCH_ENGINE_ERR);
        } else if ($ret['status'] != 0) {
            json_err_return(SEARCH_ENGINE_NOFIND_HINTS);
        } else {
            echo json_encode($ret);
        }
        break;
    //case GET_RECOMMENDS_CMD_ID:
    //    $keywords = $_REQUEST['wd'];
    //    if (isset($keywords) == false || $keywords == '') {
    //        json_err_return();
    //        break;
    //    }

    //    //trim the keyword
    //    $trimmed_keywords = trim($keywords);
    //    $keyword_ary = explode(" ", $trimmed_keywords);
    //    get_search_recommends($keyword_ary);
    //    break;
    case GET_SEARCH_CMD_ID:
        $keywords = $_REQUEST['wd'];
        if (isset($keywords) == false || $keywords == '') {
            json_err_return(PARAMETER_ERROR);
            break;
        }

        $page_num = (int)$_REQUEST['p'];
        if (isset($page_num) == false || $page_num < 0) {
            json_err_return(PARAMETER_ERROR);
            break;
        }

        $result_count = (int)$_REQUEST['c'];
        if (isset($result_count) == false || $result_count <= 0) {
            json_err_return(PARAMETER_ERROR);
            break;
        }


        //trim the keyword
        $trimmed_keywords = trim($keywords);
        $tmp_ary = explode(" ", $trimmed_keywords);
        $keyword_ary = array();
        foreach($tmp_ary as $v) {
            $keyword_ary[] = $v;
        }

        $search_ret = get_search_results($keyword_ary, $page_num, $result_count);
        if ($search_ret === false) {
            json_err_return(SEARCH_ENGINE_ERR);
            break;
        }

        if ($search_ret['status'] != 0) {
            json_err_return(SEARCH_ENGINE_NOFIND_SEARCH);
            break;
        }

        $proc_search_words = $search_ret['search_keywords'];

        $recommends_ret = get_search_recommends($proc_search_words);
        if ($recommends_ret == false) {
            json_err_return(SEARCH_ENGINE_ERR);
            break;
        }

        if ($recommends_ret['status'] == 0) {
            $search_ret['recommend_keywords'] = $recommends_ret['recommend_keywords'];
        }

        echo json_encode($search_ret);

        break;

    case SET_VIEW_TS_CMD_ID:
        $did = (int)$_REQUEST['did'];
        if (isset($did) == false || $did < 0) {
            json_err_return(PARAMETER_ERROR);
            break;
        }

        set_view_ts($did);
        json_empty_return();
        break;

    case SET_VOTE_TS_CMD_ID:
        $did = (int)$_REQUEST['did'];
        if (isset($did) == false || $did < 0) {
            json_err_return(PARAMETER_ERROR);
            break;
        }

        $rank = (int)$_REQUEST['r'];
        if (isset($rank) == false || $rank < 0) {
            json_err_return(PARAMETER_ERROR);
            break;
        }

        set_vote_ts($did, $rank);
        json_empty_return();
        break;

    case GET_HOT_KEYWORDS_CMD_ID:
        $result = get_hot_search_words();
        if ($result === false) {
            json_err_return(SEARCH_ENGINE_ERR);
            break;
        }
        if (count($result) === 0) {
            json_err_return(SEARCH_ENGINE_NOFIND_HOTSEARCH);
            break;
        }

        $ret_pkg = array('cmd_id' => GET_HOT_KEYWORDS_CMD_ID,
                         'status' => 0,
                         'keywords' => $result);

        echo json_encode($ret_pkg);
        break;

    default:
        json_err_return(PARAMETER_ERROR);
        break;
}


?>
