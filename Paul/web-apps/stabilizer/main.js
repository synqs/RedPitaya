
/*
 * starts and stops application with get request to start/stop_app_url
 * polls signals in function updateGraphData() (with specified update interval)
 * sends/loads parameters
*/

var CMD_LOAD_SETTINGS = 1;
var CMD_SAVE_SETTINGS = 2;

var app_id = 'stabilizer';  //must be the same as folder name
var root_url = '';
//var root_url = 'http://10.0.1.221';      // Test local
//var root_url = 'http://192.168.53.133';  // Test remote and local
//var root_url = 'http://192.168.1.100';   // Default RedPitaya IP
var start_app_url = root_url + '/bazaar?start=' + app_id;
var stop_app_url = root_url + '/bazaar?stop=';
var get_url = root_url + '/data';
var post_url = root_url + '/data';
var upload_url = root_url + '/upload_gen_ch';  // The channel number is added automatically

var update_interval = 50;              // Update interval for PC, milliseconds
var update_interval_mobdev = 500;      // Update interval for mobile devices, milliseconds 
var request_timeout = 3000;            // Milliseconds
var long_timeout = 20000;              // Milliseconds
var points_per_px = 5;                 // How many points per pixel should be drawn. Set null for unlimited (will disable client side decimation).
var xdecimal_places = 2;               // Number of decimal places for the xmin/xmax values. Maximum supported are 12.
var range_offset = 1;                  // Percentages
var lock_off_xdecimal_places = 4;	   // Number of decimal places for lock_off tooltip
  
var xmin = 0;
var xmax = 1000000;  

var time_range_max = [130, 1000, 8, 130, 1, 8];
var range_steps = [0.5, 1, 2, 5, 10, 20, 50, 100];

var isScanMode = true;

var CMT = 53; //index of comment parameter

var plot_options = {
  colors: ['#3276B1', '#D2322D', '#009900'],    // channel1, channel2, LockOff line
  lines: { lineWidth: 1 },
  selection: { mode: 'xy' },
  zoom: { interactive: true, trigger: null },
  xaxis: { min: xmin, max: xmax },
  grid: { borderWidth: 0 },
  legend: { noColumns: 2, margin: [0, 0], backgroundColor: 'transparent' },
  touch: { autoWidth: false, autoHeight: false }
};

// Settings which should not be modified

var update_timer = null;
var zoompan_timer = null;
var downloading = false;
var trig_dragging = false;
var lock_off = 0;
var touch_last_x = 0;
var sending = false;
var send_que = false;
var use_long_timeout = false;
var trig_dragging = false;
var touch_last_y = 0;
var user_editing = false;
var app_started = false;
var last_get_failed = false;
var autorun = true;
var datasets = [];
var plot = null;
var params = {
  original: null,
  local: null
};


//Default parameters - posted after server side app is started 
var def_params = {
  en_avg_at_dec: 0
}; 
  
// On page loaded -> initialize

$(function() {
  
  // Show different buttons on touch screens    
  if(window.ontouchstart === undefined) {
    $('.btn-lg').removeClass('btn-lg');
    $('#accordion .btn, .modal .btn').addClass('btn-sm');
    $('#btn_zoompan').remove();
    $('#btn_zoomin, #btn_zoomout, #btn_pan').show();
  }
  else {
    update_interval = update_interval_mobdev;
    $('#btn_zoomin, #btn_zoomout, #btn_pan').remove();
    $('#btn_zoompan').show();
  }
  
  // Add application ID in the message from modal popup
  $('.app-id').text(app_id);
  
  
  $('#loff_canvas').on({
    'mousedown touchstart': function(evt) {
    
      if(!params.original) {
        return;
      }
    
      trig_dragging = true;
      $('input, select', '#accordion').blur();
      mouseDownMove(this, evt);
      evt.preventDefault();
      return false;
    },
    'mousemove touchmove': function(evt) {
      if(! trig_dragging) {
        return;
      }
      mouseDownMove(this, evt);
      evt.preventDefault();
      return false;
    },
    'mouseup mouseout touchend': mouseUpOut
  });
  
  // Disable all controls until the params state is loaded for the first time 
  $('input, select, button', '.container, .onoffswitch-checkbox').prop('disabled', true);
  
  
  $('input,select', '#accordion').on('focus', function() {
    user_editing = true;
  });
  
  $('#comment').on('focus', function() {
	user_editing = true;
  });
  
  $('#op_mode').on('change', function() { 
	  if(!app_started)
		  return;
      params.local[this.id] = ($(this).is(':checked') ? 0 : 1);
	  isScanMode =  params.local[this.id];
	  if(isScanMode){
		  $('#btn_zoomin, #btn_zoomout, #btn_pan, #btn_zoompan, #range_x_minus, #range_x_plus, #offset_x_minus, #offset_x_plus, #btn_lawake').prop('disabled', true);
	  }else{
		  $('#btn_zoomin, #btn_zoomout, #btn_pan, #btn_zoompan, #range_x_minus, #range_x_plus, #offset_x_minus, #offset_x_plus, #btn_lawake').prop('disabled', false);
	  }
      sendParams(); 
  });
  
  $('#lock_off')
  .on('focus paste', function() {
    $(this).parent().addClass('input-group');
    $('#apply_lock_off').show();
  })
  .on('blur', function() {
    $('#apply_lock_off').hide();
    $(this).parent().removeClass('input-group');
    
    var loff = parseLocalFloat($(this).val());
    if(! isNaN(loff)) {
      lock_off = loff;
      redrawPlot();
      sendParams();
    }
    user_editing = false;
  })
  .on('change', function() {
    $(this).blur();
  })
  .on('keypress', function(e) {
    if(e.keyCode == 13) {
      $(this).blur();
    }
  });

  // Events binding for range controls
  
  $('#range_x_minus, #range_x_plus').on('click', function() {
	if(isScanMode)
	  return;
    var nearest = $(this).data('nearest');
    
    if(nearest && plot) {
      var options = plot.getOptions();
      var axes = plot.getAxes();
      var min = (options.xaxes[0].min !== null ? options.xaxes[0].min : axes.xaxis.min);
      var max = (options.xaxes[0].max !== null ? options.xaxes[0].max : axes.xaxis.max);
      var unit = $(this).data('unit');
      
      // Convert nanoseconds to milliseconds.
      if(unit == 'ns') {
        nearest /= 1000;
      }
      min = 0;
      max = nearest;

      options.xaxes[0].min = min;
      options.xaxes[0].max = max;

      plot.setupGrid();
      plot.draw();
      
      params.local.xmin = min;
      params.local.xmax = max;       
      
      updateRanges();
      $(this).tooltip($(this).prop('disabled') === true ? 'hide' : 'show');
      sendParams(true);
    }
  });
  
  $('#range_y_minus, #range_y_plus').on('click', function() {
    var nearest = $(this).data('nearest');
    
    if(nearest && plot) {
      var options = plot.getOptions();
      var axes = plot.getAxes();
      var min = (options.yaxes[0].min !== null ? options.yaxes[0].min : axes.yaxis.min);
      var max = (options.yaxes[0].max !== null ? options.yaxes[0].max : axes.yaxis.max);
      var unit = $(this).data('unit');
      
      // Convert millivolts to volts.
      if(unit == 'mV') {
        nearest /= 1000;
      }
      
      var center = (min + max) / 2;
      var half = nearest / 2;
      min = center - half;
      max = center + half;
      
      options.yaxes[0].min = min;
      options.yaxes[0].max = max;

      plot.setupGrid();
      plot.draw();
             
      updateRanges();
      $(this).tooltip($(this).prop('disabled') === true ? 'hide' : 'show');
    }
  });
  
  $('#offset_x_minus, #offset_x_plus').on('click', function() {
	if(isScanMode)
	  return;
    if(plot) {
      var direction = ($(this).attr('id') == 'offset_x_minus' ? 'left' : 'right');
      var options = plot.getOptions();
      var axes = plot.getAxes();
      var min = (options.xaxes[0].min !== null ? options.xaxes[0].min : axes.xaxis.min);
      var max = (options.xaxes[0].max !== null ? options.xaxes[0].max : axes.xaxis.max);
      var offset = (max - min) * range_offset/100;
      
      if(direction == 'left') {
        min -= offset;
        max -= offset;
      }
      else {
        min += offset;
        max += offset;
      }
      
      options.xaxes[0].min = 0>min ? 0 : min;
      options.xaxes[0].max = max;

      plot.setupGrid();
      plot.draw();
      
      params.local.xmin = min;
      params.local.xmax = max;
             
      updateRanges();
      sendParams(true);
    }
  });
  
  $('#offset_y_minus, #offset_y_plus').on('click', function() {
    if(plot) {
      var direction = ($(this).attr('id') == 'offset_y_minus' ? 'down' : 'up');
      var options = plot.getOptions();
      var axes = plot.getAxes();
      var min = (options.yaxes[0].min !== null ? options.yaxes[0].min : axes.yaxis.min);
      var max = (options.yaxes[0].max !== null ? options.yaxes[0].max : axes.yaxis.max);
      var offset = (max - min) * range_offset/100;
      
      if(direction == 'down') {
        min -= offset;
        max -= offset;
      }
      else {
        min += offset;
        max += offset;
      }
      
      options.yaxes[0].min = min;
      options.yaxes[0].max = max;

      plot.setupGrid();
      plot.draw();
             
      updateRanges();
    }
  });
  
  // Events binding for signal generator
  
  $('#target_channel').on('change', function() { onDropdownChange($(this), 'target_channel'); });
  
  $('#gen_ampl')
    .on('focus paste', function() {
      $(this).parent().addClass('input-group');
      $('#apply_gen_ampl').show();
    })
    .on('blur', function() {
      $('#apply_gen_ampl').hide();
      $(this).parent().removeClass('input-group');
      
      var val = parseLocalFloat($(this).val());
      if(! isNaN(val)) {
        params.local.gen_sig_amp = val;
        sendParams();
      }
      else {
        $(this).val(params.local.gen_sig_amp)
      }
      user_editing = false;
    })
    .on('change', function() {
      $(this).blur();
    })
    .on('keypress', function(e) {
      if(e.keyCode == 13) {
        $(this).blur();
      }
    });
    
/*  $('#gen_freq')
    .on('focus paste', function() {
      $(this).parent().addClass('input-group');
      $('#apply_gen_freq').show();
    })
    .on('blur', function() {
      $('#apply_gen_freq').hide();
      $(this).parent().removeClass('input-group');
      
      var val = parseLocalFloat($(this).val());
      if(! isNaN(val)) {
        params.local.gen_sig_freq = val;
        sendParams();
      }
      else {
        $(this).val(params.local.gen_sig_freq)
      }
      user_editing = false;
    })
    .on('change', function() {
      $(this).blur();
    })
    .on('keypress', function(e) {
      if(e.keyCode == 13) {
        $(this).blur();
      }
    });*/
  
  $('#gen_dc_off')
    .on('focus paste', function() {
      $(this).parent().addClass('input-group');
      $('#apply_gen_dc_off').show();
    })
    .on('blur', function() {
      $('#apply_gen_dc_off').hide();
      $(this).parent().removeClass('input-group');
      
      var val = parseLocalFloat($(this).val());
      if(! isNaN(val)) {
        params.local.gen_sig_dc_off = val;
        sendParams();
      }
      else {
        $(this).val(params.local.gen_sig_dc_off)
      }
      user_editing = false;
    })
    .on('change', function() {
      $(this).blur();
    })
    .on('keypress', function(e) {
      if(e.keyCode == 13) {
        $(this).blur();
      }
    });    
  

//Events binding for PID Controller
 
 $('#pid_11_enable, #pid_12_enable, #pid_21_enable, #pid_22_enable, #pid_121_enable').on('change', function() { 
   params.local[this.id] = ($(this).is(':checked') ? 1 : 0);
   sendParams(); 
 });

 $('#pid_11_rst, #pid_12_rst, #pid_21_rst, #pid_22_rst, #pid_121_rst').on('change', function() { 
   params.local[this.id] = ($(this).is(':checked') ? 1 : 0);
   sendParams(); 
 });
 
 // PID 11 Setpoint, Kp, Ki, Kd
 $('#pid_11_sp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_11_sp').show();
   })
   .on('blur', function() {
     $('#apply_pid_11_sp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_11_sp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_11_sp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_11_kp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_11_kp').show();
   })
   .on('blur', function() {
     $('#apply_pid_11_kp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_11_kp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_11_kp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_11_ki')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_11_ki').show();
   })
   .on('blur', function() {
     $('#apply_pid_11_ki').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_11_ki = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_11_ki)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_11_damping')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_11_damping').show();
   })
   .on('blur', function() {
     $('#apply_pid_11_damping').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_11_damping = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_11_damping)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });
 

 $('#pid_11_kd')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_11_kd').show();
   })
   .on('blur', function() {
     $('#apply_pid_11_kd').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_11_kd = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_11_kd)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 // PID 12 Setpoint, Kp, Ki, Kd
 $('#pid_12_sp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_12_sp').show();
   })
   .on('blur', function() {
     $('#apply_pid_12_sp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_12_sp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_12_sp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_12_kp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_12_kp').show();
   })
   .on('blur', function() {
     $('#apply_pid_12_kp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_12_kp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_12_kp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_12_ki')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_12_ki').show();
   })
   .on('blur', function() {
     $('#apply_pid_12_ki').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_12_ki = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_12_ki)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_12_damping')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_12_damping').show();
   })
   .on('blur', function() {
     $('#apply_pid_12_damping').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_12_damping = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_12_damping)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });
 
 $('#pid_12_kd')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_12_kd').show();
   })
   .on('blur', function() {
     $('#apply_pid_12_kd').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_12_kd = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_12_kd)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 // PID 21 Setpoint, Kp, Ki, Kd
 $('#pid_21_sp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_21_sp').show();
   })
   .on('blur', function() {
     $('#apply_pid_21_sp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_21_sp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_21_sp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_21_kp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_21_kp').show();
   })
   .on('blur', function() {
     $('#apply_pid_21_kp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_21_kp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_21_kp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_21_ki')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_21_ki').show();
   })
   .on('blur', function() {
     $('#apply_pid_21_ki').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_21_ki = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_21_ki)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_21_damping')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_21_damping').show();
   })
   .on('blur', function() {
     $('#apply_pid_21_damping').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_21_damping = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_21_damping)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });
 
 $('#pid_21_kd')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_21_kd').show();
   })
   .on('blur', function() {
     $('#apply_pid_21_kd').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_21_kd = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_21_kd)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 // PID 22 Setpoint, Kp, Ki, Kd
 $('#pid_22_sp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_22_sp').show();
   })
   .on('blur', function() {
     $('#apply_pid_22_sp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_22_sp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_22_sp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_22_kp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_22_kp').show();
   })
   .on('blur', function() {
     $('#apply_pid_22_kp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_22_kp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_22_kp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_22_ki')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_22_ki').show();
   })
   .on('blur', function() {
     $('#apply_pid_22_ki').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_22_ki = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_22_ki)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_22_damping')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_22_damping').show();
   })
   .on('blur', function() {
     $('#apply_pid_22_damping').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_22_damping = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_22_damping)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });
 
 $('#pid_22_kd')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_22_kd').show();
   })
   .on('blur', function() {
     $('#apply_pid_22_kd').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_22_kd = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_22_kd)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });
 

 // PID 121 Setpoint, Kp, Ki, Kd
 $('#pid_121_sp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_121_sp').show();
   })
   .on('blur', function() {
     $('#apply_pid_121_sp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_121_sp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_121_sp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_121_kp')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_121_kp').show();
   })
   .on('blur', function() {
     $('#apply_pid_121_kp').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_121_kp = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_121_kp)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_121_ki')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_121_ki').show();
   })
   .on('blur', function() {
     $('#apply_pid_121_ki').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_121_ki = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_121_ki)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });

 $('#pid_121_damping')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_121_damping').show();
   })
   .on('blur', function() {
     $('#apply_pid_121_damping').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_121_damping = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_121_damping)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });
 
 $('#pid_121_kd')
   .on('focus paste', function() {
     $(this).parent().addClass('input-group');
     $('#apply_pid_121_kd').show();
   })
   .on('blur', function() {
     $('#apply_pid_121_kd').hide();
     $(this).parent().removeClass('input-group');
     
     var val = parseLocalFloat($(this).val());
     if(! isNaN(val)) {
       params.local.pid_121_kd = val;
       sendParams();
     }
     else {
       $(this).val(params.local.pid_121_kd)
     }
     user_editing = false;
   })
   .on('change', function() {
     $(this).blur();
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });
 
 $('#comment')
   .on('blur', function() {
	 var keys = Object.keys(params.local);
     params.local[keys[CMT]] = -CMT;
     Object.defineProperty(params.local, $(this).val(),
    	        Object.getOwnPropertyDescriptor(params.local, keys[CMT]));
     delete params.local[keys[CMT]];
     sendParams();
     user_editing = false;
   })
   .on('keypress', function(e) {
     if(e.keyCode == 13) {
       $(this).blur();
     }
   });
 
  // Modals
  
  $('#modal_err, #modal_app').modal({ show: false, backdrop: 'static', keyboard: false });
  $('#modal_upload').modal({ show: false });
  
  $('#btn_switch_app').on('click', function() {
    var newapp_id = $('#new_app_id').text();
    if(newapp_id.length) {
      location.href = location.href.replace(app_id, newapp_id);
    }
  });
  
  $('.btn-app-restart').on('click', function() {
    location.reload();
  });
  
  $('#btn_retry_get').on('click', function() {
    $('#modal_err').modal('hide');
    updateGraphData();
  });
  
  $('.btn-close-modal').on('click', function() {
    $(this).closest('.modal').modal('hide');
  });
  
  // Reset the upload form to prevent browser caching of the uploaded file name
  $('#upload_form')[0].reset();
    
  // Other event bindings
  
  $('#loff_tooltip').tooltip({
    title: '',
    placement: 'auto top',
    animation: false
  });
  
  $('.btn').on('click', function() {
    var btn = $(this);
    setTimeout(function() { btn.blur(); }, 10);
  });
  
  $('#btn_toolbar .btn').on('blur', function() {
    $(this).removeClass('active');
  });
  
  $(document).on('click', '#accordion > .panel > .panel-heading', function(event) {
    $(this).next('.panel-collapse').collapse('toggle');
    event.stopImmediatePropagation();
  });
  
  // Tooltips for range buttons
  $('#range_x_minus, #range_x_plus, #range_y_minus, #range_y_plus').tooltip({
    container: 'body'
  });
  
  // Load first data
  updateGraphData();
  
  // Stop the application when page is unloaded
  window.onbeforeunload = function() { 
    $.ajax({
      url: stop_app_url,
      async: false
    });
  };
          
});

$(document).ready(function(){
    //$('[data-toggle="tooltip"]').tooltip();
    $('[data-toggle="tooltip"]').tooltip({container: 'body'});
});

function startApp() {
  $.get(
    start_app_url
  )
  .done(function(dresult) {
    if(dresult.status == 'ERROR') {
      showModalError((dresult.reason ? dresult.reason : 'Could not start the application.'), true);
    }
    else {
      $.post(
        post_url, 
        JSON.stringify({ datasets: { params: def_params } })
      )
      .done(function(dresult) {
        app_started = true;
        updateGraphData();      
      })
      .fail(function() {
        showModalError('Could not initialize the application with default parameters.', false, true);
      });
    }
  })
  .fail(function() {
    showModalError('Could not start the application.', true);
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

//main loop
function updateGraphData() {
  if(downloading) {
    return;
  }
  if(update_timer) {
    clearTimeout(update_timer);
    update_timer = null;
  }
  downloading = true;
  
  // Send params if there are any unsent changes
  sendParams();
  
  if(sending) {
  	update_timer = setTimeout(function() {
          updateGraphData();
      }, update_interval);
  }
  
  var long_timeout_used = use_long_timeout;
  
  $.ajax({
    url: get_url,
    timeout: (use_long_timeout ? long_timeout : request_timeout),
    cache: false
  })
  .done(function(dresult) {
    last_get_failed = false;
    
    if(dresult.status === 'ERROR') {
      if(! app_started) {
        startApp();
      }
      else {
        showModalError((dresult.reason ? dresult.reason : 'Application error.'), true, true);
      }
    }
    else if(dresult.datasets !== undefined && dresult.datasets.params !== undefined) {
      // Check if the application started on the server is the same as on client
      if(app_id !== dresult.app.id) {
        if(! app_started) {
          startApp();
        }
        else {
          $('#new_app_id').text(dresult.app.id);
          $('#modal_app').modal('show');
        }
        return;
      }
      
      app_started = true;
      
      datasets = [];
      for(var i=0; i<dresult.datasets.g1.length; i++) {
        dresult.datasets.g1[i].color = i;
        dresult.datasets.g1[i].label = 'Channel ' + (i+1);          
        datasets.push(dresult.datasets.g1[i]);
      }
      
      if(! plot) {
        initPlot(dresult.datasets.params);
      }
      else {
        // Apply the params state received from server if not in edit mode
        if(! user_editing) {
          loadParams(dresult.datasets.params);
          
        }
        // Time units must be always updated
        else {
          updateTimeUnits(dresult.datasets.params);
        }
        
        // Force X min/max
        if(dresult.datasets.params.forcex_flag == 1) {
          var options = plot.getOptions();
          options.xaxes[0].min = dresult.datasets.params.xmin;
          options.xaxes[0].max = dresult.datasets.params.xmax;
        }
        
        // Redraw the plot using new datasets
        plot.setData(filterData(datasets, plot.width(), options.yaxes[0].min, options.yaxes[0].max));
        plot.setupGrid();
        plot.draw();
      }
      
      if(! trig_dragging) {
          updateLockOffSlider();
      }
      
      updateRanges();

      if(autorun || dresult.status === 'AGAIN') {
    	update_timer = setTimeout(function() {
            updateGraphData();
        }, update_interval);
      }
    }
    else {
      showModalError('Wrong application data received.', true, true);
    }
  })
  .fail(function(jqXHR, textStatus, errorThrown) {
    if(last_get_failed) {
      showModalError('Data receiving failed.<br>Error status: ' + textStatus, true, true);
      last_get_failed = false;
    }
    else {
      last_get_failed = true;
      downloading = false;
      updateGraphData();  // One more try
    }
  })
  .always(function() {
    if(! last_get_failed) {
      downloading = false;
      
      if(params.local) {
    	$('#accordion').find('input,select').prop('disabled', false);
        $('.btn').not('#range_y_plus, #range_y_minus, #range_x_plus, #range_x_minus').prop('disabled', false);
        $('.onoffswitch-checkbox').prop('disabled', false);
        if(isScanMode){
  		  $('#btn_zoomin, #btn_zoomout, #btn_pan, #btn_zoompan, #range_x_minus, #range_x_plus, #offset_x_minus, #offset_x_plus, #btn_lawake').prop('disabled', true);
        }else{
  		  $('#btn_zoomin, #btn_zoomout, #btn_pan, #btn_zoompan, #range_x_minus, #range_x_plus, #offset_x_minus, #offset_x_plus, #btn_lawake').prop('disabled', false);
  	  	}
      }
    }
    
    if(long_timeout_used) {
      use_long_timeout = false;
    }
  });
}

function initPlot(init_params) {
  var plot_holder = $('#plot_holder');
  var ymax = init_params.gui_reset_y_range / 2;
  var ymin = ymax * -1;
  
  // Load received params
  loadParams(init_params);
  
  // When xmin/xmax are null, the min/max values of received data will be used. For ymin/ymax use the gui_reset_y_range param.
  $.extend(true, plot_options, {
    xaxis: { min: null, max: null },
    yaxis: { min: ymin, max: ymax }
  });
  
  // Local optimization    
  var filtered_data = filterData(datasets, plot_holder.width(), ymin, ymax);

  // Plot first creation and drawing
  plot = $.plot(
    plot_holder, 
    filtered_data,
    plot_options
  );
  // Selection
  plot_holder.on('plotselected', function(event, ranges) {
  
    if(isScanMode)
    	return;
    
	// Clamp the zooming to prevent eternal zoom
    if(ranges.xaxis.to - ranges.xaxis.from < 0.00001) {
      ranges.xaxis.to = ranges.xaxis.from + 0.00001;
    }
    if(ranges.yaxis.to - ranges.yaxis.from < 0.00001) {
      ranges.yaxis.to = ranges.yaxis.from + 0.00001;
    }

    // Do the zooming
    plot = $.plot(
      plot_holder, 
      getData(ranges.xaxis.from, ranges.xaxis.to),
      $.extend(true, plot_options, {
        xaxis: { min: ranges.xaxis.from, max: ranges.xaxis.to },
        yaxis: { min: ranges.yaxis.from, max: ranges.yaxis.to }
      })
    );
    
    params.local.xmin = parseFloat(ranges.xaxis.from.toFixed(xdecimal_places));
    params.local.xmax = parseFloat(ranges.xaxis.to.toFixed(xdecimal_places));
    
    updateLockOffSlider();
    
    sendParams(true);
    
  });
  
  // Zoom / Pan
  plot_holder.on('plotzoom plotpan touchmove touchend', function(event) {
	if(isScanMode)
	  return;
	    
    if(zoompan_timer) {
      clearTimeout(zoompan_timer);
      zoompan_timer = null;
    }
    
    zoompan_timer = setTimeout(function() {
      zoompan_timer = null;
      
      var xaxis = plot.getAxes().xaxis;
      params.local.xmin = parseFloat(xaxis.min.toFixed(xdecimal_places));
      params.local.xmax = parseFloat(xaxis.max.toFixed(xdecimal_places));
      
      updateLockOffSlider();
      
      sendParams(true);
              
    }, 250);
  });
}

//////////
//send, load params

function onDropdownChange(that, param_name, do_get) {
  params.local[param_name] = parseInt(that.val());
  sendParams(do_get);
  that.blur();
  user_editing = false;
}

function loadParams(orig_params) {
  if(! $.isPlainObject(orig_params)) {
    return;
  }
  
  // Same data in local and original params
  params.original = $.extend({}, orig_params);
  params.local = $.extend({}, params.original);
  
  $('#op_mode').prop('checked', (params.original.op_mode ? false : true));
  
  $('#target_channel').val(params.original.target_channel);
  $('#gen_ampl').val(floatToLocalString(params.original.gen_sig_amp));
  $('#gen_dc_off').val(floatToLocalString(params.original.gen_sig_dc_off));
  
  $('#lock_off').val(floatToLocalString(params.original.lock_off));
  lock_off = params.original.lock_off;
  
//PID
  $('#pid_11_enable').prop('checked', (params.original.pid_11_enable ? true : false));
  $('#pid_12_enable').prop('checked', (params.original.pid_12_enable ? true : false));
  $('#pid_21_enable').prop('checked', (params.original.pid_21_enable ? true : false));
  $('#pid_22_enable').prop('checked', (params.original.pid_22_enable ? true : false));
  $('#pid_121_enable').prop('checked', (params.original.pid_121_enable ? true : false));


  $('#pid_11_rst').prop('checked', (params.original.pid_11_rst ? true : false));
  $('#pid_12_rst').prop('checked', (params.original.pid_12_rst ? true : false));
  $('#pid_21_rst').prop('checked', (params.original.pid_21_rst ? true : false));
  $('#pid_22_rst').prop('checked', (params.original.pid_22_rst ? true : false));
  $('#pid_121_rst').prop('checked', (params.original.pid_121_rst ? true : false));

  $('#pid_11_sp').val(params.original.pid_11_sp);
  $('#pid_11_kp').val(params.original.pid_11_kp);
  $('#pid_11_ki').val(params.original.pid_11_ki);
  $('#pid_11_damping').val(params.original.pid_11_damping);
  $('#pid_11_kd').val(params.original.pid_11_kd);
  $('#pid_12_sp').val(params.original.pid_12_sp);
  $('#pid_12_kp').val(params.original.pid_12_kp);
  $('#pid_12_ki').val(params.original.pid_12_ki);
  $('#pid_12_damping').val(params.original.pid_12_damping);
  $('#pid_12_kd').val(params.original.pid_12_kd);
  $('#pid_21_sp').val(params.original.pid_21_sp);
  $('#pid_21_kp').val(params.original.pid_21_kp);
  $('#pid_21_ki').val(params.original.pid_21_ki);
  $('#pid_21_damping').val(params.original.pid_21_damping);
  $('#pid_21_kd').val(params.original.pid_21_kd);
  $('#pid_22_sp').val(params.original.pid_22_sp);
  $('#pid_22_kp').val(params.original.pid_22_kp);
  $('#pid_22_ki').val(params.original.pid_22_ki);
  $('#pid_22_damping').val(params.original.pid_22_damping);
  $('#pid_22_kd').val(params.original.pid_22_kd);
  $('#pid_121_sp').val(params.original.pid_121_sp);
  $('#pid_121_kp').val(params.original.pid_121_kp);
  $('#pid_121_ki').val(params.original.pid_121_ki);
  $('#pid_121_damping').val(params.original.pid_121_damping);
  $('#pid_121_kd').val(params.original.pid_121_kd);
  
  var keys = Object.keys(params.original);
  $('#comment').val(keys[CMT]);
  
  isScanMode = params.original.op_mode==0;
  
  if(isScanMode){
	  $('#btn_zoomin, #btn_zoomout, #btn_pan, #btn_zoompan, #range_x_minus, #range_x_plus, #offset_x_minus, #offset_x_plus, #btn_lawake').prop('disabled', true);
  }else{
	  $('#btn_zoomin, #btn_zoomout, #btn_pan, #btn_zoompan, #range_x_minus, #range_x_plus, #offset_x_minus, #offset_x_plus, #btn_lawake').prop('disabled', false);
  }
  
  if(params.original.en_avg_at_dec) {
    $('#btn_avg').removeClass('btn-default').addClass('btn-primary');
  }
  else {
    $('#btn_avg').removeClass('btn-primary').addClass('btn-default');
  }
  
  updateTimeUnits(orig_params);
  $('#ytitle').show();
}

function updateTimeUnits(new_params) {
  if(! $.isPlainObject(new_params)) {
    return;
  } 
  params.original.time_units = params.local.time_units = new_params.time_units;

  var timeu_lbl = (params.original.time_units == 0 ? 'μs' : (params.original.time_units == 1 ? 'ms' : 's'));
  $('#xtitle').text('Time [ ' + timeu_lbl + ' ]');
}

function isParamChanged() {
  if(params.original) {
    for(var key in params.original) {
      if(params.original[key] != params.local[key]) {
        return true;
      }
    }
  }
  return false;
}

function sendParams(refresh_data, force_send) {
  if(sending || (force_send !== true && !isParamChanged())) {
    send_que = sending;
    return;
  }
  
  sending = true;
  
  $.ajax({
    type: 'POST',
    url: post_url,
    data: JSON.stringify({ datasets: { params: params.local } }),
    timeout: (use_long_timeout ? long_timeout : request_timeout),
    cache: false
  })
  .done(function(dresult) {
    // OK: Load the params received as POST result
    if(dresult.datasets !== undefined && dresult.datasets.params !== undefined) {
    
      if(dresult.datasets.params.forcex_flag == 1) {
        var options = plot.getOptions();
        
        options.xaxes[0].min = dresult.datasets.params.xmin;
        options.xaxes[0].max = dresult.datasets.params.xmax;
       
        plot.setupGrid();
        plot.draw();
      }      
    
      loadParams(dresult.datasets.params);
      
      updateLockOffSlider();
      
      if(refresh_data && !downloading) {
        updateGraphData();
      } 
    }
    else if(dresult.status == 'ERROR') {
      showModalError((dresult.reason ? dresult.reason : 'Error while sending data (E1).'), false, true, true);
      send_que = false;
    }
    else {
      showModalError('Error while sending data (E2).', false, true, true);
    }
  })
  .fail(function() {
    showModalError('Error while sending data (E3).', false, true, true);
  })
  .always(function() {
    sending = false;
    user_editing = false;
    
    if(send_que) {
      send_que = false;
      setTimeout(function(refresh_data) {
        sendParams(refresh_data);
      }, 100);
    }
  });
}

////prepare data for plotting, redraw

// Use only data for selected channels and do downsampling (data decimation), which is required for 
// better performance. On the canvas cannot be shown too much graph points. 
function filterData(dsets, points, ymin, ymax) {
  var filtered = [];
  var num_of_channels = 2;

  for(var l=0; l<num_of_channels; l++) {
    if(! $('#btn_ch' + (l+1)).data('checked')) {
      continue;
    }

    i = Math.min(l, dsets.length - 1);

    filtered.push({ color: dsets[i].color, label: dsets[i].label, data: [] });
    
    if(points_per_px === null || dsets[i].data.length > points * points_per_px) {
      var step = Math.ceil(dsets[i].data.length / (points * points_per_px));
      var k = 0;
      for(var j=0; j<dsets[i].data.length; j++) {
        if(k > 0 && ++k < step) {
          continue;
        }
        filtered[filtered.length - 1].data.push(dsets[i].data[j]);
        k = 1;
      }
    }
    else {
      filtered[filtered.length - 1].data = dsets[i].data.slice(0);
    }
  }
  
  filtered = addLockOffDataSet(filtered, ymin, ymax);
  return filtered;
}

// Add a data series for the LockOff level line
function addLockOffDataSet(dsets, ymin, ymax) {

  if(!isScanMode || params.original.gen_sig_freq<=0  || params.original.gen_sig_amp<=0) 
	return dsets;

  // Don't add LockOff dataset if LockOff level is outside the visible area...
  if(plot) {
    var xaxis = plot.getAxes().xaxis;
    if(lockValue2Time(lock_off) < xaxis.min || lockValue2Time(lock_off) > xaxis.max) {
      return dsets;
    }
  }
  var mid = (ymax + ymin)/2;
  var yrange = ymax - ymin;
  dsets[dsets.length] = { color: 2, lines: {lineWidth: 3} , data: [[lockValue2Time(lock_off), mid - yrange*0.02], [lockValue2Time(lock_off), mid + yrange*0.02]], shadowSize: 1 };
  
  return dsets;
}

function redrawPlot() {
  if(! downloading) {
    if(! plot) {
      updateGraphData();
    }
    else {
      var options = plot.getOptions();
      plot = $.plot(
        plot.getPlaceholder(), 
        filterData(datasets, plot.width(), options.yaxes[0].min, options.yaxes[0].max),
        $.extend(true, plot_options, {
          xaxis: { min: options.xaxes[0].min, max: options.xaxes[0].max },
          yaxis: { min: options.yaxes[0].min, max: options.yaxes[0].max }
        })
      );
      updateLockOffSlider();
    }
  }
}

/////////////
//plot tools and actions (zooming, dragging, leave awake..)

function leaveAwake(btn){
  //dont notice app
  window.onbeforeunload = function() { 
  };
  document.location.href = '/index.html';
}

function setVisibleChannels(btn) {
  var other_btn = $(btn.id == 'btn_ch1' ? '#btn_ch2' : '#btn_ch1');
  var btn = $(btn);
  var checked = !btn.data('checked');
  
  btn.data('checked', checked).toggleClass('btn-default btn-primary');
  
  // At least one button must be checked, so that at least one graph will be visible.
  if(! checked) {
    other_btn.data('checked', true).removeClass('btn-default').addClass('btn-primary');
  }
  redrawPlot();
}

function autoscaleY() {
  if(! plot) {
    return;
  }
  
  var options = plot.getOptions();
  var axes = plot.getAxes();
  
  // Set Y scale to data min/max + 10%
  options.yaxes[0].min = (axes.yaxis.datamin < 0 ? axes.yaxis.datamin * 1.1 : axes.yaxis.datamin - axes.yaxis.datamin * 0.1); 
  options.yaxes[0].max = (axes.yaxis.datamax > 0 ? axes.yaxis.datamax * 1.1 : axes.yaxis.datamax + axes.yaxis.datamax * 0.1);

  plot.setupGrid();
  plot.draw();
  
  updateRanges();
}


function resetZoom() {
  if(! plot) {
    return;
  }
  
  $('#btn_ch1, #btn_ch2').data('checked', true).removeClass('btn-default').addClass('btn-primary');
  
  var ymax = params.original.gui_reset_y_range / 2;
  var ymin = ymax * -1;
  
  if(!isScanMode){
	  $.extend(true, plot_options, {
		    xaxis: { min: null, max: null },
		    yaxis: { min: ymin, max: ymax }
	  });
		       
	  var options = plot.getOptions();
	  options.xaxes[0].min = null;
	  options.xaxes[0].max = null;
	  options.yaxes[0].min = ymin;
	  options.yaxes[0].max = ymax;
	  params.local.xmin = 0;
	  params.local.xmax = 131;
	  params.local.time_units = 0;
  }else{
	  $.extend(true, plot_options, {
		    yaxis: { min: ymin, max: ymax }
	  });
	  var options = plot.getOptions();
	  options.yaxes[0].min = ymin;
	  options.yaxes[0].max = ymax;
  }
  
  plot.setupGrid();
  plot.draw();
  
  sendParams(true, true);
}

function updateZoom() {
  if(! plot || isScanMode) {
    return;
  }
  
  params.local.xmin = 0;
  params.local.xmax = time_range_max[params.local.time_range];

  var axes = plot.getAxes();
  var options = plot.getOptions();
  
  options.xaxes[0].min = params.local.xmin;
  options.xaxes[0].max = params.local.xmax;
  options.yaxes[0].min = axes.yaxis.min;
  options.yaxes[0].max = axes.yaxis.max;
  
  plot.setupGrid();
  plot.draw();

  sendParams(true, true);
}

function selectTool(toolname) {
  if(isScanMode)
    return;
  $('#selzoompan .btn').removeClass('btn-primary').addClass('btn-default');
  $(this).toggleClass('btn-default btn-primary');

  if(toolname == 'zoomin') {
    enableZoomInSelection();
  }
  if(toolname == 'zoomout') {
    enableZoomOut();
  }
  else if(toolname == 'pan') {
    enablePanning();
  }
}

function enableZoomInSelection() {
  if(isScanMode)
    return;
  
  if(plot_options.hasOwnProperty('selection')) {
    return;
  }
  
  var plot_pholder = plot.getPlaceholder();

  // Disable panning and zoom out
  delete plot_options.pan;
  plot_pholder.off('click.rp');
  
  // Get current min/max for both axes to use them to fix the current view
  var axes = plot.getAxes();
  
  plot = $.plot(
    plot_pholder, 
    plot.getData(),
    $.extend(true, plot_options, {
      selection: { mode: 'xy' },
      xaxis: { min: axes.xaxis.min, max: axes.xaxis.max },
      yaxis: { min: axes.yaxis.min, max: axes.yaxis.max }
    })
  );
}

function enableZoomOut() {
  if(isScanMode)
    return;
  
  var plot_pholder = plot.getPlaceholder();
  
  plot_pholder.on('click.rp', function(event) {
    var offset = $(event.target).offset();
    
    plot.zoomOut({
      center: { left: event.pageX - offset.left, top: event.pageY - offset.top },
      amount: 1.2
    });
  });
  
  // Disable zoom in selection and panning
  delete plot_options.selection;
  delete plot_options.pan;
  
  // Get current min/max for both axes to use them to fix the current view
  var axes = plot.getAxes();
  
  plot = $.plot(
    plot_pholder, 
    plot.getData(),
    $.extend(true, plot_options, {
      xaxis: { min: axes.xaxis.min, max: axes.xaxis.max },
      yaxis: { min: axes.yaxis.min, max: axes.yaxis.max }
    })
  );
}

function enablePanning() {
  if(isScanMode)
    return;
  
  if(plot_options.hasOwnProperty('pan')) {
    return;
  }
  
  var plot_pholder = plot.getPlaceholder();
  
  // Disable selection zooming and zoom out
  delete plot_options.selection;
  plot_pholder.off('click.rp');
  
  // Get current min/max for both axes to use them to fix the current view
  var axes = plot.getAxes();
  
  plot = $.plot(
    plot_pholder, 
    plot.getData(),
    $.extend(true, plot_options, {
      pan: { interactive: true },
      xaxis: { min: axes.xaxis.min, max: axes.xaxis.max },
      yaxis: { min: axes.yaxis.min, max: axes.yaxis.max }
    })
  );
}

function getData(from, to) {
  if(isScanMode)
   	return;
  
  var rangedata = new Array();
  for(var i=0; i<datasets.length; i++) {
    if(! $('#btn_ch' + (i+1)).data('checked')) {
      continue;
    }
    rangedata.push({ color: datasets[i].color, label: datasets[i].label, data: [] });
    for(var j=0; j<datasets[i].data.length; j++) {
      if(datasets[i].data[j][0] > to) {
        break;
      }
      if(datasets[i].data[j][0] >= from) {
        rangedata[rangedata.length - 1].data.push(datasets[i].data[j]);
      }
    }
  }
  rangedata = filterData(rangedata, (plot ? plot.width() : $('plot_holder').width()));
  return rangedata;
}

//calculate voltage from time based on falling edge (25-75ms) of the triangle signal
function time2LockValue(x){
	return (params.original.gen_sig_dc_off + params.original.gen_sig_amp * (0.5 - 0.001 * (x - 25) * 2 * params.original.gen_sig_freq)).toFixed(lock_off_xdecimal_places);
}

function lockValue2Time(loff){
	if(params.original.gen_sig_amp<=0 || params.original.gen_sig_freq<=0)
		return 25;
	return -((loff - params.original.gen_sig_dc_off)/params.original.gen_sig_amp - 0.5)/(0.001 * 2 * params.original.gen_sig_freq) + 25;
}

//////////
//lock off gui element

function mouseDownMove(that, evt) {
  if(!isScanMode || params.original.gen_sig_freq<=0  || params.original.gen_sig_amp<=0) 
	  return;
  var x;
  user_editing = true;
  
  if(evt.type.indexOf('touch') > -1) {
    x = evt.originalEvent.touches[0].clientX - that.getBoundingClientRect().left - plot.getPlotOffset().left;
    touch_last_x = x;
  }
  else {
    x = evt.clientX - that.getBoundingClientRect().left - plot.getPlotOffset().left;
  }
  updateLockOffSlider(x);
  
  $('#loff_tooltip').data('bs.tooltip').options.title = time2LockValue(plot.getAxes().xaxis.c2p(x).toFixed(lock_off_xdecimal_places));
  $('#loff_tooltip').tooltip('show');
}

function mouseUpOut(evt) {
  if(!isScanMode || params.original.gen_sig_freq<=0  || params.original.gen_sig_amp<=0) 
    return;
  
  if(trig_dragging) {
    trig_dragging = false;
    
    var x;
    if(evt.type.indexOf('touch') > -1) {
      //y = evt.originalEvent.touches[0].clientY - this.getBoundingClientRect().top - plot.getPlotOffset().top;
      x = touch_last_x;
    }
    else {
      x = evt.clientX - this.getBoundingClientRect().left - plot.getPlotOffset().left;
    }
    redrawPlot();
    sendParams();
  }
  else {
    user_editing = false;
  }
  $('#loff_tooltip').tooltip('hide');
}

function updateLockOffSlider(x, update_input) {
  if(! plot) {
    return;
  }
  
  var canvas = $('#loff_canvas')[0];
  var context = canvas.getContext('2d');
  var plot_offset = plot.getPlotOffset();
  var xmax = params.original.xmax;
  var xmin = params.original.xmin;
  
  var loff_x = lockValue2Time(lock_off);

  // If LockOff level is outside the predefined xmin/xmax, change the level
  
  if(isScanMode){
	  if(loff_x < xmin) {
		  loff_x = xmin;
	  }
	  else if(loff_x > xmax) {
		  loff_x = xmax;
	  } 
  }
  
  
  if(x === undefined) {
    if(update_input !== false) {
      $('#lock_off').not(':focus').val(floatToLocalString(time2LockValue(loff_x)));
    }
    x = plot.getAxes().xaxis.p2c(loff_x);
  }
  
  lock_off = time2LockValue(parseFloat(plot.getAxes().xaxis.c2p(x).toFixed(lock_off_xdecimal_places)));   
  
  params.local.lock_off = parseFloat(lock_off);
  
  // If LockOff level is not in visible area, do not show the LockOff slider and paint the vertical line with gray
  context.clearRect(0, 0, canvas.width, canvas.height); 
  var xaxis = plot.getAxes().xaxis;
  if(loff_x < xaxis.min || loff_x > xaxis.max || !isScanMode || params.original.gen_sig_freq<=0  || params.original.gen_sig_amp<=0) {
    context.lineWidth = 1;
    context.strokeStyle = '#dddddd';
    context.stroke();
    context.beginPath();
    context.moveTo(plot_offset.left, 10);
    context.lineTo(canvas.width - plot_offset.right + 1, 10);
    context.stroke();
  }
  else {
    context.beginPath();
    context.arc(x + plot_offset.left, 10, 8, 0, 2 * Math.PI, false);
    context.fillStyle = '#009900';
    context.fill();
    context.lineWidth = 1;
    context.strokeStyle = '#007700';
    context.stroke();
    context.beginPath();
    context.moveTo(plot_offset.left, 10);
    context.lineTo(canvas.width - plot_offset.right + 1, 10);
    context.stroke();
  }
  $('#loff_tooltip').css({ left: x + plot_offset.left, top: -5});  
}

///////
//plot ranges

function updateRanges() {
  var xunit = (params.local.time_units == 0 ? 'μs' : (params.local.time_units == 1 ? 'ms' : 's'));
  var yunit = 'V';
  var axes = plot.getAxes();
  var xrange = axes.xaxis.max - axes.xaxis.min;
  var yrange = axes.yaxis.max - axes.yaxis.min;
  var yminrange = 5e-3;  // Volts
  var ymaxrange = params.original.gui_reset_y_range;
  var xmaxrange = 10.0;  // seconds
  var xminrange = 20e-9; // seconds
  var decimals = 0;

  if(xunit == 'μs' && xrange < 1) {
    xrange *= 1000;
    xunit = 'ns';
  }
  if(xrange < 1) {
    decimals = 1;
  }
  var seconds = (xunit == 'ns' ? 1e-9 : ( xunit == 'μs' ? 1e-6 : (xunit == 'ms' ? 1e-3 : 1)));
  
  if(yrange < 1) {
    yunit = 'mV';
    yrange *= 1000;
    ymaxrange *= 1000;
    yminrange *= 1000;
  }

  $('#range_x').html(+(Math.round(xrange + "e+" + decimals) + "e-" + decimals) + ' ' + xunit);
  $('#range_y').html(Math.floor(yrange) + ' ' + yunit);
  
  var nearest_x = getNearestRanges(xrange);
  var nearest_y = getNearestRanges(yrange);
      
  // X limitations 
  if(nearest_x.next * seconds > xmaxrange) {
    nearest_x.next = null;
    $('#range_x_plus').prop('disabled', true);
  }
  else {
    $('#range_x_plus').prop('disabled', false);
  }
  if(nearest_x.prev * seconds < xminrange) {
    nearest_x.prev = null;
    $('#range_x_minus').prop('disabled', true);
  }
  else {
    $('#range_x_minus').prop('disabled', false);
  }
  
  $('#range_x_minus').data({ nearest: nearest_x.prev, unit: xunit }).data('bs.tooltip').options.title = nearest_x.prev;
  $('#range_x_plus').data({ nearest: nearest_x.next, unit: xunit }).data('bs.tooltip').options.title = nearest_x.next;
  
  // Y limitations
  if(nearest_y.next - nearest_y.prev >= ymaxrange) {
    nearest_y.next = null;
    $('#range_y_plus').prop('disabled', true);
  }
  else {
    $('#range_y_plus').prop('disabled', false);
  }
  if(nearest_y.prev < yminrange) {
    nearest_y.prev = null;
    $('#range_y_minus').prop('disabled', true);
  }
  else {
    $('#range_y_minus').prop('disabled', false);
  }
  
  $('#range_y_minus').data({ nearest: nearest_y.prev, unit: yunit }).data('bs.tooltip').options.title = nearest_y.prev;
  $('#range_y_plus').data({ nearest: nearest_y.next, unit: yunit }).data('bs.tooltip').options.title = nearest_y.next;
}

function getNearestRanges(number) {
  var log10 = Math.floor(Math.log(number) / Math.LN10); 
  var normalized = number / Math.pow(10, log10);    
  var i = 0;
  var prev = null;
  var next = null;
  
  while(i < range_steps.length - 1) {
    var ratio = range_steps[i+1] / normalized;
    if(ratio > 0.99 && ratio < 1.01) {
      prev = range_steps[i];
      next = range_steps[i+2];
      break;
    }
    if(range_steps[i] < normalized && normalized < range_steps[i+1]) {
      prev = range_steps[i];
      next = range_steps[i+1];
      break;
    }
    i++;
  }

  return { 
    prev: prev * Math.pow(10, log10), 
    next: next * Math.pow(10, log10) 
  };
}


/////////
function getLocalDecimalSeparator() {
  var n = 1.1;
  return n.toLocaleString().substring(1,2);
}

function parseLocalFloat(num) {
  return +(num.replace(getLocalDecimalSeparator(), '.'));
}

function floatToLocalString(num) {
  // Workaround for a bug in Safari 6 (reference: https://github.com/mleibman/SlickGrid/pull/472)
  //return num.toString().replace('.', getLocalDecimalSeparator());
  return (num + '').replace('.', getLocalDecimalSeparator());
}

function shortenFloat(value) {
  return value.toFixed(Math.abs(value) >= 10 ? 1 : 3);
}

function convertHz(value) {
  var unit = '';
  var decsep = getLocalDecimalSeparator();
  
  if(value >= 1e6) {
    value /= 1e6;
    unit = '<span class="unit">MHz</span>';
  }
  else if(value >= 1e3) {
    value /= 1e3;
    unit = '<span class="unit">kHz</span>';
  }
  else {
    unit = '<span class="unit">Hz</span>';
  }

  // Fix to 4 decimal digits in total regardless of the decimal point placement
  var eps = 1e-2;
  if (value >= 100 - eps) {
    value = value.toFixed(1);
  }
  else if  (value >= 10 - eps) {
    value = value.toFixed(2);
  }
  else {
    value = value.toFixed(3);
  }
  
  value = (value == 0 ? '---' + decsep + '-' : floatToLocalString(value));
  return value + unit;
}

function convertSec(value) {
  var unit = '';
  var decsep = getLocalDecimalSeparator();
  
  if(value < 1e-6) {
    value *= 1e9;
    unit = '<span class="unit">ns</span>';
  }
  else if(value < 1e-3) {
    value *= 1e6;
    unit = '<span class="unit">μs</span>';
  }
  else if(value < 1) {
    value *= 1e3
    unit = '<span class="unit">ms</span>';
  }
  else {
    unit = '<span class="unit">s</span>';
  }

  // Fix to 4 decimal digits in total regardless of the decimal point placement
  var eps = 1e-2;
  if (value >= 100 - eps) {
    value = value.toFixed(1);
  }
  else if  (value >= 10 - eps) {
    value = value.toFixed(2);
  }
  else {
    value = value.toFixed(3);
  }
  
  value = (value == 0 ? '---' + decsep + '-' : floatToLocalString(value));
  return value + unit;
}

//////////////
//Settings Import/Export/Load/Save

function showSettingsImportForm() {
  $('#uploaded_file').prop('disabled', false);
  var file_elem = $('#uploaded_file');
  var fcontent_elem = $('#uploaded_file_content');
  var hint_elem = $('#upload_form .help-block');

  fcontent_elem.val('');
  hint_elem.html('Select the rps file to upload.');
  $('#upload_form')[0].reset();
  $('#modal_upload_label span').text("settings");
  $('#uploaded_file').attr("accept",".rps");

  if(file_elem[0].files === undefined) {
	file_elem.hide().parent().addClass('has-error');
	hint_elem.html('Your browser is too old and do not support this feature.');
  }
  else {
	file_elem.show().parent().removeClass('has-error');
	file_elem.off('change').on('change', function() {
      var file = this.files[0];
      var freader = new FileReader();
    
      // File validation
      //var size = file.size;
      var type = file.type;
      if(false) {
        file_elem.parent().addClass('has-error');
        hint_elem.html('Wrong file type: ' + file.type);
        $('#upload_form')[0].reset();
      }
      else {
        file_elem.parent().removeClass('has-error');
        hint_elem.html('File size: ' + file.size + ' bytes');
        freader.onload = function(data) {
          fcontent_elem.val(this.result);
        };
        freader.readAsText(file);
      }
    });
  }

  $('#modal_upload').modal('show');

  return false;
}

function startUpload() {
  var mbody = $('#modal_upload .modal-body');
  var file_elem = $('#uploaded_file');
  var hint_elem = $('#upload_form .help-block');
  var channel = $('#modal_upload_label span').text();
  if(mbody.data('errtimer')) {
      clearTimeout(mbody.data('errtimer'));
      mbody.removeData('errtimer');
  }
  if(! $('#uploaded_file').val().length) {
    mbody.addClass('alert-danger').data('errtimer', setTimeout(function() { 
    $('#modal_upload .modal-body').removeClass('alert-danger'); 
    }, 1000));
    return;
  }
  file_elem.hide();
  hint_elem.html('Uploading file...');
  var content = $('#uploaded_file_content').val();
  if(content === '') {
      file_elem.hide().parent().addClass('has-error');
      hint_elem.html('Error while uploading the file.');
      return;
  }
  else {
	try{
  	  var obj = JSON.parse(content);
  	}catch(err){
  	  file_elem.show();
  	  hint_elem.html('Error while parsing file.');
  	  file_elem.parent().addClass('has-error');
  	  return;
  	}
   	var new_params = obj.datasets.params;
   	if(new_params === undefined){
	  file_elem.show();
  	  hint_elem.html('Error while parsing file.');
  	  file_elem.parent().addClass('has-error');
  	  return;
   	}
   	user_editing = true;
	var options = plot.getOptions();
	options.yaxes[0].min = obj.datasets.ymin;
	options.yaxes[0].max = obj.datasets.ymax;
	options.xaxes[0].min = obj.datasets.xmin;
	options.xaxes[0].max = obj.datasets.xmax;
	loadParams(new_params);
	redrawPlot();
	$('#upload_form')[0].reset();
	$('#modal_upload').modal('hide');
	sendParams(false, true);
    return;
  }
  file_elem.show();
  hint_elem.html('Error while uploading the file.');
  file_elem.parent().addClass('has-error');
  return;
}

function exportSettings() {
  if(!app_started){
	showModalError('Error while saving settings.', false, false, true);
	return false;
  }
  var options = plot.getOptions();
  var blob = new Blob([JSON.stringify({ datasets: { params: params.original, y_min: options.yaxes[0].min, y_max: options.yaxes[0].max} })], {type: "text/plain;charset=utf-8"});
  saveAs(blob, "settings.rps");
  return false;
}

function loadSettings() {
	if(!app_started || params.local===undefined) {
		showModalError('Error while saving settings.', false, false, true);
		return false;
	}
	++params.local.cmd_cnt;
	params.local.cmd = CMD_LOAD_SETTINGS;
	sendParams();
}

function saveSettings() {
	if(!app_started || params.local===undefined) {
		showModalError('Error while saving settings.', false, false, true);
		return false;
	}
	++params.local.cmd_cnt;
	params.local.cmd = CMD_SAVE_SETTINGS;
	sendParams();
}