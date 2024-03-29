require "util"

NEED_TO_PLOT = {}


function co_wrapper(f, ... )
   local forward = {...} 
   local co = coroutine.create(function () f(table.unpack(forward)) end) 
   return function ()
      local status, res  = coroutine.resume(co)
      return res
   end
end

function sanfname(s)
   return string.gsub(s,"/","__")
end

function sigbkg(tbl)
   hist_glob = tbl[1]
   sig = tbl[2]
   bkg = tbl[3]
   options = tbl[4]
   subpath = tbl[5]
   annotations=tbl.annotations
   sig_set = get_histos(sig, hist_glob)
   bkg_set = get_histos(bkg, hist_glob)
   save = tbl.save
   if save == nil then save = true end
   for key,set in pairs(sig_set) do
      transforms.sort_integral(bkg_set[key])
      dp = DrawPad:new()
      legend = plotting.new_legend(dp)
      plotting.stack(dp, 0 , bkg_set[key], options:plot_title(key))
      plotting.simple(dp, 0 , set, Options:new())
      plotting.add_to_legend(legend,  set)
      plotting.add_to_legend(legend,  bkg_set[key])
      plotting.add_legend_to_pad(legend, dp, 0);
      if save then
         plotting.save_pad(dp,
                           OUTPUT_BASE_PATH .. "/"
                           .. subpath .. "/"..
                           sanfname(key) .. ".pdf")
      end
      coroutine.yield(key)
   end
end

function simple_hist(tbl)
   hist_glob = tbl[1]
   sig = tbl[2]
   normed=tbl[3]
   options = tbl[4]
   subpath = tbl[5]
   titlefunc = tbl.title_func
   text=tbl.extra_text
   sig_set = get_histos(sig, hist_glob)
   save = tbl.save
   if save == nil then save = true end
   for key,set in pairs(sig_set) do
      dp, pidx = table.unpack(tbl.pad or {DrawPad:new(), 0})
      if normed then
         to_plot = transforms.norm_to(set, normed)
      else 
         to_plot = set
      end
      legend = plotting.new_legend(dp)

      if titlefunc then
         plotting.simple(dp, pidx , to_plot, options:plot_title(titlefunc(key)))
      else
         plotting.simple(dp, pidx , to_plot, options:plot_title(key))
      end
      for i, t in ipairs(text or {}) do
         annotation.draw_text(dp, pidx, t)
      end
      plotting.add_to_legend(legend,  to_plot)
      plotting.add_legend_to_pad(legend, dp, pidx);
      if save then

         plotting.save_pad(dp,
                           OUTPUT_BASE_PATH .. "/"
                           .. subpath .. "/"..
                           sanfname(key) .. ".pdf")
      end
      coroutine.yield(key)
   end
end

function multihist(tbl)
   shape = tbl.shape
   len = #tbl.plots
   subpath = tbl.subpath
   savename= OUTPUT_BASE_PATH .. "/" .. "/" .. (tbl.savename or "random") .. ".pdf"
   pad = DrawPad:new()
   pad:divide(table.unpack(shape))
   for i,args in ipairs(tbl.plots) do
      local fun=args[1]
      table.remove(args, 1)
      args.pad = {pad, i}
      args.save = false
      fun(args)
   end

   plotting.save_pad(dp, savename)
   coroutine.yield("multiplot")
end


function hist2(tbl)
   hist_glob = tbl[1]
   sig = tbl[2]
   normed=tbl[3]
   options = tbl[4]
   subpath = tbl[5]
   titlefunc = tbl.title_func
   text=tbl.extra_text
   sig_set = get_histos2(sig, hist_glob)
   save = tbl.save
   if save == nil then save = true end
   for key,set in pairs(sig_set) do
      if normed then
         to_plot = transforms.norm_to(set, normed)
      else 
         to_plot = set
      end
      dp = DrawPad:new()
      -- legend = plotting.new_legend(dp)
      if titlefunc then
         plotting.simple2(dp, 0 , to_plot, options:plot_title(titlefunc(key)))
      else
         plotting.simple2(dp, 0 , to_plot, options:plot_title(key))
      end
      for i, t in ipairs(text or {}) do
         annotation.draw_text(dp, 0, t)
      end

      if save then 
         --plotting.add_to_legend(legend,  to_plot)
         --plotting.add_legend_to_pad(legend, dp, 0);
         plotting.save_pad(dp,
                           OUTPUT_BASE_PATH .. "/"
                           .. subpath .. "/"..
                           sanfname(key) .. ".pdf")
      end

      coroutine.yield(key)
   end
end


function plot(args)
   table.insert(NEED_TO_PLOT, args)
end

function execute_deferred_plots()
   VLOG(1, "Starting execution of deferred plots")
   total = 0
   group = 0
   start_time = os.time()
   io.write(string.format("Executing %d plot groups.\n", #NEED_TO_PLOT))
   for i , args in ipairs(NEED_TO_PLOT) do
      group = group + 1
      local fun=args[1]
      table.remove(args, 1)
      for key in co_wrapper(fun, args) do
         total = total + 1
         if VERBOSITY < 2 then
            io.write(string.rep(" ", 100) .. "\r")
         end
         collectgarbage()
         io.write(string.format("Plotting [%d:%d/%d] -- %s%s", total, group,  #NEED_TO_PLOT, key,  VERBOSITY <2 and '\r' or '\n'))
         if VERBOSITY < 2 then
            io.flush()
         end
      end
      if VERBOSITY < 2 then
         io.write(string.rep(" ", 100) .. "\r")
      end
   end
   io.write(string.format("Generated %d plots in %d seconds.\n", total, os.time() - start_time))
end


colors = {
   new_color("#1f77b4"),
   new_color("#ff7f0e"),
   new_color("#2ca02c"),
   new_color("#d62728"),
   new_color("#9467bd"),
   new_color("#8c564b"),
   new_color("#e377c2"),
   new_color("#7f7f7f"),
   new_color("#bcbd22"),
   new_color("#17becf")
}

