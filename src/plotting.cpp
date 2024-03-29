#include "plotting.h"

#include <TStyle.h>

namespace rootp {
void setFillAtt(const Style &style, TAttFill *fill_att) {
    if (!(style.mode & Style::Mode::Fill)) {
        fill_att->SetFillColorAlpha(0, 0);
        return;
    }
    if (style.color) {
        auto color = style.color.value();
        fill_att->SetFillColor(color);
    } else if (style.palette_idx) {
        auto color = gStyle->GetColorPalette(style.palette_idx.value());
        fill_att->SetFillColor(color);
    }
    if (style.fill_style) {
        fill_att->SetFillStyle(style.fill_style.value());
    }
}

void setMarkAtt(const Style &style, TAttMarker *mark_att) {
    if (!(style.mode & Style::Mode::Marker)) {
        return;
    }
    if (style.color) {
        auto color = style.color.value();
        mark_att->SetMarkerColor(color);
    } else if (style.palette_idx) {
        auto color = gStyle->GetColorPalette(style.palette_idx.value());
        mark_att->SetMarkerColor(color);
    }
    if (style.marker_style) {
        mark_att->SetMarkerStyle(style.marker_style.value());
    }
    if (style.marker_size) {
        mark_att->SetMarkerSize(style.marker_size.value());
    }
}

void setLineAtt(const Style &style, TAttLine *line_att) {
    // if (!(style.mode & Style::Mode::Line)) {
    //     return;
    // }
    if (style.color) {
        auto color = style.color.value();
        line_att->SetLineColor(color);
    } else if (style.palette_idx) {
        auto color = gStyle->GetColorPalette(style.palette_idx.value());
        line_att->SetLineColor(color);
    }
    if (style.line_style) {
        line_att->SetLineStyle(style.line_style.value());
    }
    if (style.line_width) {
        line_att->SetLineWidth(style.line_width.value());
    }
}

std::string getStyleString(const Style &style) {
    std::string ret;
    if (style.mode & Style::Mode::Fill) {
        ret += "HIST";
    }
    if (style.mode & Style::Mode::Line) {
        ret += "E";
    }
    return ret;
}

void applyCommonOptions(TVirtualPad *pad, const CommonOptions &opts) {
    if (opts.logx) pad->SetLogx();
    if (opts.logy) pad->SetLogy();
}
void savePadTo(TVirtualPad *p, const std::filesystem::path &path) {
    std::filesystem::path parent = path.parent_path();
    if (!std::filesystem::is_directory(parent)) {
        std::filesystem::create_directories(parent);
    }
    vPrint(VerbosityLevel::Medium, "Saving to path {}\n", path.string());
    p->SaveAs(path.string().c_str());
}
void saveDrawPad(const DrawPad &p, const std::filesystem::path &path) {
    savePadTo(p.pad.get(), path);
}
void saveDrawPad(const DrawPad &p, const std::string &path) {
    saveDrawPad(p, std::filesystem::path(path));
}

float getMin(TH1 *hist, float cutoff) { return hist->GetMinimum(cutoff); }
float getMin(THStack *hist, float cutoff) {
    double min = 10000000;
    for (TObject *o : *hist->GetHists()) {
        auto h = static_cast<TH1 *>(o);
        min = std::min(h->GetMinimum(cutoff), min);
    }
    return min;
}

template <typename T>
void auto_range(DrawPad &dp, T *other, float cutoff = 0.000001) {
    if (!dp.init) {
        dp.master = other;
        dp.init = true;
    }
    std::visit(
        [&](const auto &x) {
            assert(x);
            float other_min = getMin(other, cutoff);
            float other_max = other->GetMaximum();
            float old_min = getMin(x, cutoff);
            float old_max = x->GetMaximum();
            dp.min_y = std::min(old_min, other_min);
            dp.max_y = std::max(old_max, other_max);
            x->SetMinimum(0.9 * dp.min_y);
            x->SetMaximum(1.1 * dp.max_y);
        },
        dp.master);
    //  dp.master->SetMinimum(dp.min_y);
    //  dp.master->SetMaximum(dp.max_y);
}

void plotStandard(DrawPad &dp, int subpad,
                  const std::vector<PlotData<TH1>> &data,
                  const CommonOptions &options) {
    auto pad = dp.pad.get();
    pad->cd(subpad);
    int i = 0;
    for (const auto &d : data) {
        applyCommonOptions(d.hist.get(), options);
        applyCommonOptions(pad, options);
        dp.objects.emplace_back(d.hist);
        setMarkAtt(d.style, d.hist.get());
        setLineAtt(d.style, d.hist.get());
        setFillAtt(d.style, d.hist.get());
        if (d.hist->GetEntries() > 0) {
            d.hist->Draw((getStyleString(d.style) + " Same").c_str());
            auto_range(dp, d.hist.get());
        }
        d.hist->SetMinimum(dp.min_y);
        d.hist->SetMaximum(dp.max_y);
        ++i;
        pad->Update();
    }
}

void plotStandard2(DrawPad &dp, int subpad,
                   const std::vector<PlotData<TH2>> &data,
                   const CommonOptions &options) {
    auto pad = dp.pad.get();
    pad->cd(subpad);
    int i = 0;
    for (const auto &d : data) {
        applyCommonOptions(d.hist.get(), options);
        applyCommonOptions(pad, options);
        dp.objects.emplace_back(d.hist);
        setMarkAtt(d.style, d.hist.get());
        setLineAtt(d.style, d.hist.get());
        setFillAtt(d.style, d.hist.get());
        if (d.hist->GetEntries() > 0) {
            d.hist->Draw("Same COLZ");
            auto_range(dp, d.hist.get());
        }
        d.hist->SetMinimum(dp.min_y);
        d.hist->SetMaximum(dp.max_y);
        ++i;
        pad->Update();
    }
}

void addLegendToPad(TLegend *legend, DrawPad &p, int subpad) {
    p.pad->cd(subpad);
    setupLegend(legend);
}
TLegend *newLegend(DrawPad &p) {
    auto legend = new TLegend;
    p.objects.emplace_back(legend);
    return legend;
}

void plotStack(DrawPad &dp, int subpad, const std::vector<PlotData<TH1>> &data,
               const CommonOptions &options) {
    if (data.empty()) return;
    auto pad = dp.pad.get();
    pad->cd(subpad);
    auto stack = new THStack;
    bool needs_fill = false;
    for (const auto &d : data) {
        applyCommonOptions(d.hist.get(), options);
        setMarkAtt(d.style, d.hist.get());
        setLineAtt(d.style, d.hist.get());
        setFillAtt(d.style, d.hist.get());
        stack->Add(d.hist.get());
        dp.objects.emplace_back(d.hist);
        if (d.style.mode & Style::Mode::Fill) {
            needs_fill = true;
        }
    }
    std::string hist_options = "";
    if (dp.init) hist_options += "SAME";
    if (needs_fill) hist_options += "hist";

    stack->Draw(hist_options.c_str());

    auto_range(dp, stack);

    applyCommonOptions(pad, options);
    applyCommonOptions(stack, options);
}

// void executePlot(DrawPad &p, const PlotDescription &plot) {
//     p.pad->Divide(plot.structure.first, plot.structure.second);
//     for (const auto &[pad_idx, desc] : plot.pads) {
//         for (const auto &[func, data] : desc.drawers) {
//             func(p, pad_idx, data, desc.options);
//         }
//     }
// }

void setupLegend(TLegend *legend) {
    legend->SetX1(0.65);
    legend->SetY1(0.90 - 0.05 * (1 + legend->GetNRows()));
    legend->SetX2(0.90);
    legend->SetY2(0.90);
    legend->SetTextSize(0.025);
    legend->SetHeader("Samples", "C");
    legend->Draw();
}

void DrawPad::divide(int r, int c) { pad->Divide(r, c); }

namespace transforms {
std::shared_ptr<TH1> normalize(const TH1 *hist, float val) {
    auto ret = std::shared_ptr<TH1>(static_cast<TH1 *>(hist->Clone()));
    auto integral = ret->Integral();
    ret->Scale(val / integral);
    return ret;
}
}  // namespace transforms
}  // namespace rootp
