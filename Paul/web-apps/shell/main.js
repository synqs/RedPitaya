/**
 * @brief RedPitaya Web-Shell main module
 * @author Paul Hill
 */
var app_id = 'shell';
var root_url = '';
var start_app_url = root_url + '/bazaar?start=' + app_id;
var stop_app_url = root_url + '/bazaar?stop=';
var get_url = root_url + '/data';
var post_url = root_url + '/data';
var request_timeout = 3000;            // Milliseconds
var long_timeout = 20000;              // Milliseconds
var loggedIn = false;
var params = {
  local: null
};
var psswd;
var commands = [];
var c_index = 0;
var current_input = '';
var update_interval = 1000; //ms
var short_update_interval = 100; //ms
$(function(){
	$('#modal_err').modal({ show: false, backdrop: 'static', keyboard: false });
    
    $('.btn-app-restart').on('click', function() {
      location.reload();
    });
    
    $('#btn_retry_get').on('click', function() {
      $('#modal_err').modal('hide');
      //updateGraphData();
    });
    
    $('.btn-close-modal').on('click', function() {
      $(this).closest('.modal').modal('hide');
    });
	window.onbeforeunload = function() { 
		$.ajax({
	      url: stop_app_url,
	      async: false
	    });
	};
	$('#shell_input').on('keypress', function(e){
		if(e.keyCode == 13){
			if(!loggedIn){
				psswd = $('#shell_input').val();
				params.local = {[psswd]:0, echo:0};
			}else{
				var cmnd = $('#shell_input').val();
				addCommand(cmnd, true);
				params.local = {[psswd]:0, [cmnd]:0};
			}
			$.ajax({
				type: 'POST',
			    url: post_url,
			    data: JSON.stringify({ datasets: { params: params.local}}),
			    timeout: request_timeout,
			    cache: false
			})
			.done(function(dresult){
				if(dresult.status == 'ERROR' || dresult.datasets == undefined || dresult.datasets.params == undefined) {
					showModalError((dresult.reason ? dresult.reason : 'Error.'),false,true,false);
				}else {
					if(dresult.datasets.params.psswd==0){
						if(!loggedIn){
							addCommand('That went wrong. Try again.');
						}else
							showModalError('Error.',false,true,false);
					}
					else{
						if(!loggedIn){
							loggedIn = true;
							addCommand('Hello!');
							$('#shell_input').attr('placeholder','');
							retrieveOutput();
						}
					}
				}
			})
			.fail(function(){
				showModalError('Error.',false,true,false);
			});
			e.target.value = '';
		}
	});
	$('#shell_input').on('keyup', function(e){
		if(e.keyCode == 13){
			e.target.value = '';
		}
	});
	$('#shell_input').on('keypress', function(e){
		if(e.keyCode == 38 && c_index>0){
			if(c_index==commands.length)
				current_input = e.target.value;
			--c_index;
			e.target.value = commands[c_index];
		}
		if(e.keyCode == 40 && c_index<commands.length){
			++c_index;
			e.target.value = commands[c_index];
		}
		if(e.keyCode == 40 && c_index==commands.length && c_index!=0){
			e.target.value = current_input;
		}
	});
	$('#shell_input').attr('placeholder', 'Enter login-password: [will be transferred unencrypted!]');
	startApp();
});
  
function startApp(){
	$.get(
			start_app_url
	)
	.done(function(dresult){
		if(dresult.status == 'ERROR') {
			showModalError((dresult.reason ? dresult.reason : 'Could not start the application.'),false,true,false);
		}
	})
	.fail(function(){
		showModalError('Could not start the application.',false,true,false);
	})
}
  
function addCommand(cmd, add){
	$('#shell').html($('#shell').html()+'<p><span class="prompt1">red</span><span class="prompt2">pitaya></span><span class="cmd">  '+cmd+'</span></p>');
	if(add){
		commands.push(cmd);
		c_index = commands.length;
	}
	$('#shell').scrollTop($('#shell').prop('scrollHeight'));
}

function retrieveOutput(){
	 $.ajax({
	      url: get_url,
	      timeout: request_timeout,
	      cache: false
	 })
	 .done(function(dresult) {
		 if(dresult.status == 'ERROR' || dresult.datasets == undefined || dresult.datasets.g1 == undefined) {
				showModalError((dresult.reason ? dresult.reason : 'Error.'),false,true,false);
		 }else{
			 var interval = update_interval;
			 switch(dresult.datasets.g1[0].data[0][0]){
			 case -1:
				 showModalError((dresult.reason ? dresult.reason : 'Error.'),false,true,false);
				 return;
			 case 0:
				 break;
			 case 2:
				 interval = short_update_interval;
			 case 1:
				 var i=0;
				 var result_str = '';
				 while(dresult.datasets.g1[0].data[i][1]!=0){
					 result_str += String.fromCharCode(dresult.datasets.g1[0].data[i][1]);
					 ++i;
				 }
				 $('#shell').html($('#shell').html()+'<p><span class="cmd">  '+result_str+'</span></p>');
				 $('#shell').scrollTop($('#shell').prop('scrollHeight'));
				 break;
			 }
			 update_timer = setTimeout(function() {
		            retrieveOutput();
		          }, interval);
		 }
	 });
}
  
function showModalError(err_msg, retry_btn, restart_btn, ignore_btn) {
    var err_modal = $('#modal_err');
    
    err_modal.find('#btn_retry_get')[retry_btn ? 'show' : 'hide']();
    err_modal.find('.btn-app-restart')[restart_btn ? 'show' : 'hide']();
    err_modal.find('#btn_ignore')[ignore_btn ? 'show' : 'hide']();
    err_modal.find('.modal-body').html(err_msg);
    err_modal.modal('show');
  }