/* This file is part of Clementine.
   Copyright 2010, David Sansome <me@davidsansome.com>

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "scriptdialog.h"
#include "scriptmanager.h"
#include "ui_scriptdialog.h"

#include <QPainter>
#include <QPushButton>
#include <QtDebug>

const int ScriptDelegate::kIconSize = 64;
const int ScriptDelegate::kPadding = 6;
const int ScriptDelegate::kItemHeight = kIconSize + kPadding*2;
const int ScriptDelegate::kLinkSpacing = 10;

ScriptDelegate::ScriptDelegate(QObject* parent)
  : QStyledItemDelegate(parent),
    bold_metrics_(bold_)
{
  bold_.setBold(true);
  bold_metrics_ = QFontMetrics(bold_);

  link_.setUnderline(true);
}

void ScriptDelegate::paint(
    QPainter* p, const QStyleOptionViewItem& opt,
    const QModelIndex& index) const {
  // Draw the background
  const QStyleOptionViewItemV3* vopt = qstyleoption_cast<const QStyleOptionViewItemV3*>(&opt);
  const QWidget* widget = vopt->widget;
  QStyle* style = widget->style() ? widget->style() : QApplication::style();
  style->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, p, widget);

  // Get the data
  QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
  const QString title = index.data(Qt::DisplayRole).toString();
  const QString description = index.data(ScriptManager::Role_Description).toString();
  const bool is_enabled = index.data(ScriptManager::Role_IsEnabled).toBool();

  // Calculate the geometry
  QRect icon_rect(opt.rect.left() + kPadding, opt.rect.top() + kPadding,
                  kIconSize, kIconSize);
  QRect title_rect(icon_rect.right() + kPadding, icon_rect.top(),
                   opt.rect.width() - icon_rect.width() - kPadding*3,
                   bold_metrics_.height());
  QRect description_rect(title_rect.left(), title_rect.bottom(),
                         title_rect.width(), opt.rect.bottom() - kPadding - title_rect.bottom());

  // Draw the icon
  p->drawPixmap(icon_rect, icon.pixmap(kIconSize, is_enabled ? QIcon::Normal : QIcon::Disabled));

  // Disabled items get greyed out
  if (is_enabled) {
    p->setPen(opt.palette.color(QPalette::Text));
  } else {
    const bool light = opt.palette.color(QPalette::Base).value() > 128;
    const QColor color = opt.palette.color(QPalette::Dark);
    p->setPen(light ? color.darker(150) : color.lighter(125));
  }

  // Draw the title
  p->setFont(bold_);
  p->drawText(title_rect, Qt::AlignLeft | Qt::AlignVCenter, title);

  // Draw the description
  p->setFont(opt.font);
  p->drawText(description_rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, description);
}

QSize ScriptDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const {
  return QSize(100, kItemHeight);
}


ScriptDialog::ScriptDialog(QWidget* parent)
  : QDialog(parent),
    ui_(new Ui_ScriptDialog),
    manager_(NULL)
{
  ui_->setupUi(this);
  connect(ui_->enable, SIGNAL(clicked()), SLOT(Enable()));
  connect(ui_->disable, SIGNAL(clicked()), SLOT(Disable()));
  connect(ui_->settings, SIGNAL(clicked()), SLOT(Settings()));
  connect(ui_->details, SIGNAL(clicked()), SLOT(Details()));

  ui_->tab_widget->setCurrentIndex(0);
  ui_->list->setItemDelegate(new ScriptDelegate(this));
}

ScriptDialog::~ScriptDialog() {
  delete ui_;
}

void ScriptDialog::SetManager(ScriptManager* manager) {
  manager_ = manager;
  ui_->list->setModel(manager);

  connect(manager, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
          SLOT(DataChanged(QModelIndex,QModelIndex)));
  connect(manager, SIGNAL(LogLineAdded(QString)), SLOT(LogLineAdded(QString)));
  connect(ui_->list->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
          SLOT(CurrentChanged(QModelIndex)));

  foreach (const QString& html, manager->log_lines()) {
    LogLineAdded(html);
  }
}

void ScriptDialog::CurrentChanged(const QModelIndex& index) {
  if (!index.isValid()) {
    ui_->enable->setEnabled(false);
    ui_->disable->setEnabled(false);
    ui_->settings->setEnabled(false);
    ui_->details->setEnabled(false);
    return;
  }

  const bool is_enabled = index.data(ScriptManager::Role_IsEnabled).toBool();
  ui_->enable->setEnabled(!is_enabled);
  ui_->disable->setEnabled(is_enabled);
  ui_->settings->setEnabled(is_enabled);
  ui_->details->setEnabled(true);
}

void ScriptDialog::DataChanged(const QModelIndex&, const QModelIndex&) {
  CurrentChanged(ui_->list->currentIndex());
}

void ScriptDialog::Enable() {
  manager_->Enable(ui_->list->currentIndex());
}

void ScriptDialog::Disable() {
  manager_->Disable(ui_->list->currentIndex());
}

void ScriptDialog::Settings() {
  manager_->ShowSettingsDialog(ui_->list->currentIndex());
}

void ScriptDialog::Details() {

}

void ScriptDialog::LogLineAdded(const QString& html) {
  // This is such a hack, I'm sorry.
  if (html.startsWith("<font color=\"red\">") &&
      ui_->tab_widget->currentWidget() != ui_->console_tab) {
    ui_->tab_widget->SetTabAlert(ui_->tab_widget->indexOf(ui_->console_tab));
  }
  ui_->console->append(html);
}
