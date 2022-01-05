/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2022 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2022 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "delay_analysis.h"
#include "ui_delay_analysis.h"
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_point_data.h>
#include <qwt_spline.h>

DelayAnalysisDialog::DelayAnalysisDialog(QWidget *parent)
    : QDialog(parent),
      m_measurer(new Measurer(this)), m_ui(new Ui::DelayAnalysis)
{
    m_ui->setupUi(this);
}

DelayAnalysisDialog::~DelayAnalysisDialog()
{
}

bool DelayAnalysisDialog::computeResult(const FmBank::Instrument &ins)
{
    if (!m_measurer->doComputation(ins, m_result))
        return false;

    updateDisplay();
    return true;
}

class DelayAnalysisDialog::PlotData : public QwtSyntheticPointData
{
    const double m_step;
    const std::vector<double> &m_data;
public:
    PlotData(double xstep, const std::vector<double> &data);
    size_t size() const override;
    double x(uint index) const override;
    double y(double x) const override;
};

void DelayAnalysisDialog::updateDisplay()
{
    const Measurer::DurationInfo &result = m_result;

    Ui::DelayAnalysis &ui = *m_ui;

    double xMaxOn = result.amps_timestep * result.amps_on.size();
    double yMaxOn = 0;
    for(double y : result.amps_on)
        yMaxOn = (y > yMaxOn) ? y : yMaxOn;

    double xMaxOff = result.amps_timestep * result.amps_off.size();
    double yMaxOff = 0;
    for(double y : result.amps_off)
        yMaxOff = (y > yMaxOff) ? y : yMaxOff;

    const QColor colorGrid = Qt::gray;
    const QColor colorCurve = Qt::green;
    const QColor colorMarker = Qt::yellow;
    const QColor colorBg = Qt::darkBlue;

    QwtPlot *plotOn = ui.plotDelayOn;
    plotOn->setAxisScale(QwtPlot::xBottom, 0.0, xMaxOn);
    plotOn->setAxisScale(QwtPlot::yLeft, 0.0, yMaxOn);
    plotOn->setCanvasBackground(colorBg);

    QwtPlot *plotOff = ui.plotDelayOff;
    plotOff->setAxisScale(QwtPlot::xBottom, 0.0, xMaxOff);
    plotOff->setAxisScale(QwtPlot::yLeft, 0.0, yMaxOff);
    plotOff->setCanvasBackground(colorBg);

    QwtPlotCurve *curveOn = m_curveOn;
    if(!curveOn) {
        curveOn = m_curveOn = new QwtPlotCurve;
        curveOn->setPen(colorCurve, 0.0, Qt::SolidLine);
        curveOn->attach(plotOn);
    }

    QwtPlotCurve *curveOff = m_curveOff;
    if(!curveOff) {
        curveOff = m_curveOff = new QwtPlotCurve;
        curveOff->setPen(colorCurve, 0.0, Qt::SolidLine);
        curveOff->attach(plotOff);
    }

    QwtPlotGrid *gridOn = m_gridOn;
    if(!gridOn) {
        gridOn = m_gridOn = new QwtPlotGrid;
        gridOn->setPen(colorGrid, 0.0, Qt::DotLine);
        gridOn->attach(plotOn);
    }

    QwtPlotGrid *gridOff = m_gridOff;
    if(!gridOff) {
        gridOff = m_gridOff = new QwtPlotGrid;
        gridOff->setPen(colorGrid, 0.0, Qt::DotLine);
        gridOff->attach(plotOff);
    }

    PlotData *dataOn = new PlotData(result.amps_timestep, result.amps_on);
    delete m_dataOn;
    m_dataOn = dataOn;
    curveOn->setData(dataOn);

    PlotData *dataOff = new PlotData(result.amps_timestep, result.amps_off);
    delete m_dataOff;
    m_dataOff = dataOff;
    curveOff->setData(dataOff);

    QwtPlotMarker *markerOn = m_markerOn;
    if(!markerOn) {
        markerOn = m_markerOn = new QwtPlotMarker;
        markerOn->setLineStyle(QwtPlotMarker::VLine);
        markerOn->setLinePen(colorMarker, 0.0, Qt::SolidLine);
        markerOn->attach(plotOn);
    }
    markerOn->setXValue(result.ms_sound_kon * 1e-3);

    QwtPlotMarker *markerOff = m_markerOff;
    if(!markerOff) {
        markerOff = m_markerOff = new QwtPlotMarker;
        markerOff->setLineStyle(QwtPlotMarker::VLine);
        markerOff->setLinePen(colorMarker, 0.0, Qt::SolidLine);
        markerOff->attach(plotOff);
    }
    markerOff->setXValue(result.ms_sound_koff * 1e-3);

    plotOn->replot();
    plotOff->replot();

    updateLabels();
}

void DelayAnalysisDialog::updateLabels()
{
    const Measurer::DurationInfo &result = m_result;

    Ui::DelayAnalysis &ui = *m_ui;
    QwtPlot *plotOn = ui.plotDelayOn;
    QwtPlot *plotOff = ui.plotDelayOff;

    double yMaxOn = plotOn->axisInterval(QwtPlot::yLeft).maxValue();
    double yMaxOff = plotOff->axisInterval(QwtPlot::yLeft).maxValue();

    if(plotOn)
        plotOn->setTitle(tr("Key-On Amplitude"));
    if(plotOff)
        plotOff->setTitle(tr("Key-Off Amplitude"));

    PlotData *dataOn = m_dataOn;
    PlotData *dataOff = m_dataOff;

    double yBreakOn = 0.0;
    double yBreakOff = 0.0;
    if(dataOn)
        yBreakOn = dataOn->y(result.ms_sound_kon * 1e-3);
    if(dataOff)
        yBreakOff = dataOff->y(result.ms_sound_koff * 1e-3);

    QLabel *lbOn = ui.textDelayOn;
    QLabel *lbOff = ui.textDelayOff;
    lbOn->setText(tr("Delay: %1 ms\n"
                     "Peak amplitude: %2\n"
                     "Amplitude at breaking point: %3")
                  .arg(result.ms_sound_kon)
                  .arg(yMaxOn)
                  .arg(yBreakOn));
    lbOff->setText(tr("Delay: %1 ms\n"
                      "Peak amplitude: %2\n"
                      "Amplitude at breaking point: %3")
                   .arg(result.ms_sound_koff)
                   .arg(yMaxOff)
                   .arg(yBreakOff));
}

void DelayAnalysisDialog::onLanguageChanged()
{
    m_ui->retranslateUi(this);
    updateLabels();
}

void DelayAnalysisDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        onLanguageChanged();
    QDialog::changeEvent(event);
}

DelayAnalysisDialog::PlotData::PlotData(double xstep, const std::vector<double> &data)
    : QwtSyntheticPointData(data.size()),
      m_step(xstep), m_data(data)
{
}

size_t DelayAnalysisDialog::PlotData::size() const
{
    return m_data.size();
}

double DelayAnalysisDialog::PlotData::x(uint index) const
{
    return index * m_step;
}

double DelayAnalysisDialog::PlotData::y(double x) const
{
    QPolygonF poly;
    const int maxpoints = 8;
    poly.reserve(maxpoints);

    // fraction indexing into sample array
    double frac = x / m_step;
    long index = (long)frac;

    // take neighboring samples left and right for interpolation
    for(long di = -maxpoints/2; di < maxpoints/2; ++di)
    {
        long i1 = index + di;
        QPointF p(i1 * m_step, 0);
        if(i1 >= 0)
        {
            if ((size_t)i1 >= m_data.size()) continue;
            p.setY(m_data[i1]);
        }
        poly.push_back(p);
    }

    // compute the spline using neighbour points
    QwtSpline spline;
    spline.setPoints(poly);

    // evaluate
    double interpolatedY = spline.value(x);
    return interpolatedY;
}
