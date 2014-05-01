/*
* This file is part of Octopi, an open-source GUI for pacman.
* Copyright (C) 2014 Thomas Binkau
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

#include "packagemodel.h"

#include <iostream>
#include <cassert>

#include "src/uihelper.h"
#include "src/strconstants.h"


PackageModel::PackageModel(const PackageRepository& repo, QObject *parent)
: QAbstractItemModel(parent), m_packageRepo(repo), m_displayMode(FLAT),
  m_rootItem(createDummyRoot()),
  m_sortOrder(Qt::AscendingOrder), m_sortColumn(1),
  m_filterPackagesNotInstalled(false), m_filterPackagesNotInThisGroup(""),
  m_filterColumn(-1), m_filterRegExp("", Qt::CaseInsensitive, QRegExp::RegExp),
  m_iconNotInstalled(IconHelper::getIconNonInstalled()), m_iconInstalled(IconHelper::getIconInstalled()),
  m_iconInstalledUnrequired(IconHelper::getIconUnrequired()),
  m_iconNewer(IconHelper::getIconNewer()), m_iconOutdated(IconHelper::getIconOutdated()),
  m_iconInstalledByUser(IconHelper::getIconInstalledUser()),
  m_iconInstalledUnrequiredByUser(IconHelper::getIconUnrequiredUser()),
  m_iconNewerByUser(IconHelper::getIconNewerUser()),
  m_iconOutdatedByUser(IconHelper::getIconOutdatedUser()),
  m_iconForeign(IconHelper::getIconForeignGreen()), m_iconForeignOutdated(IconHelper::getIconForeignRed())
{
}

QModelIndex PackageModel::index(int row, int column, const QModelIndex &parent) const
{
  switch (m_displayMode) {
  case FLAT: {
    if (!hasIndex(row, column, parent))
      return QModelIndex();

    if (!parent.isValid()) {
      const int adaptedRow = transformRowIndex(row, m_columnSortedlistOfPackages.size());
      if (adaptedRow >= 0 && adaptedRow < m_columnSortedlistOfPackages.size()) {
        return createIndex(row, column, (void*)m_columnSortedlistOfPackages.at(adaptedRow));
      }
    }
    return QModelIndex();
  }
  case DEPENDS_ON:
  case REQUIRED_BY: {
    const PackageItem& pkgItem = getPackageItem(parent);
    const std::size_t adaptedRow = transformRowIndex(row, pkgItem.getChildCount());
    if (adaptedRow < static_cast<std::size_t>(pkgItem.getChildCount())) {
      const PackageItem*const child = pkgItem.getChildAt(adaptedRow);
      if (child != NULL) {
        return createIndex(row, column, (void*)child);
      }
    }
    return QModelIndex();
  }
  default:
    assert(false);
    return QModelIndex();
  }
}

QModelIndex PackageModel::parent(const QModelIndex& child) const
{
  switch (m_displayMode) {
  case FLAT:
    return QModelIndex();

  case DEPENDS_ON:
  case REQUIRED_BY: {
    if (child.isValid() == false)
      return QModelIndex();

    const PackageItem*const data = static_cast<const PackageItem*const>(child.internalPointer());
    if (data == NULL || &data->getParent() == m_rootItem.get()) {
      return QModelIndex();
    }
    else {
      const PackageItem& parent = data->getParent();
      int row = 0; //FIXME: search necessary ?
      for (; row < parent.getChildCount(); ++row) {
        if (parent.getChildAt(row) == data)
          break;
      }
      return createIndex(row, 0, (void*)&parent);
    }
  }
  default:
    assert(false);
    return QModelIndex();
  }
}

int PackageModel::rowCount(const QModelIndex& parent) const
{
  if (m_displayMode == FLAT) {
    if (parent.isValid() == false) {
      return m_columnSortedlistOfPackages.size();
    }
    else return 0;
  }
  else if (m_displayMode == DEPENDS_ON || m_displayMode == REQUIRED_BY)
  {
    const PackageItem& branch = getPackageItem(parent);
    return branch.getChildCount();
  } else {
    assert(false);
    return 0;
  }
}

int PackageModel::columnCount(const QModelIndex&) const
{
  switch (m_displayMode) {
  case FLAT:
    return 5;
  case DEPENDS_ON:
  case REQUIRED_BY:
    return 5;
  default:
    return 0;
  }
}

QVariant PackageModel::data(const QModelIndex &index, int role) const
{
  if (index.isValid()) {
    switch (role) {
      case Qt::DisplayRole: {
        const PackageRepository::PackageData*const package = getData(index);
        if (package == NULL)
          return QVariant();

        switch (index.column()) {
          case ctn_PACKAGE_ICON_COLUMN:
            if (m_displayMode != FLAT) return QVariant(package->name);
            break;
          case ctn_PACKAGE_NAME_COLUMN:
            if (m_displayMode == FLAT) return QVariant(package->name);
            break;
          case ctn_PACKAGE_VERSION_COLUMN:
            return QVariant(package->version);
          case ctn_PACKAGE_REPOSITORY_COLUMN:
            return QVariant(package->repository);
          case ctn_PACKAGE_POPULARITY_COLUMN:
            if (package->popularity >= 0)
              return QVariant(package->popularityString);
            break;
          default:
            assert(false);
        }
        break;
      }
      case Qt::DecorationRole: {
        const PackageRepository::PackageData*const package = getData(index);
        if (package == NULL)
          return QVariant();

        if (index.column() == ctn_PACKAGE_ICON_COLUMN) {
          return QVariant(getIconFor(*package));
        }
        break;
      }
      case Qt::ToolTipRole:
        break;
      case Qt::StatusTipRole:
        break;
      default:
        break;
    }
  }
  return QVariant();
}

QVariant PackageModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal) {
      switch (section) {
      case ctn_PACKAGE_ICON_COLUMN:
        if (m_displayMode == FLAT) return QVariant();
        if (m_displayMode == DEPENDS_ON) return QVariant("Name (item depends on its child items)"); //FIXME: translate
        if (m_displayMode == REQUIRED_BY) return QVariant("Name (item is required by its child items)"); //FIXME: translate
        break;
      case ctn_PACKAGE_NAME_COLUMN:
        if (m_displayMode == FLAT) return QVariant(StrConstants::getName());
        return QVariant();
      case ctn_PACKAGE_VERSION_COLUMN:
        return QVariant(StrConstants::getVersion());
      case ctn_PACKAGE_REPOSITORY_COLUMN:
        return QVariant(StrConstants::getRepository());
      case ctn_PACKAGE_POPULARITY_COLUMN:
        return QVariant(StrConstants::getPopularityHeader());
      default:
        break;
      }
    }
    return QVariant(section);
  }
  return QAbstractItemModel::headerData(section, orientation, role);
}

bool PackageModel::canFetchMore(const QModelIndex& parent) const
{
  if (parent.isValid() == false || parent.row() < 0)
    return false;

  switch (m_displayMode) {
  case FLAT:
  default:
    return false;
  case DEPENDS_ON:
    return getPackageItem(parent).canFetchChildren(PackageItem::DEPENDS_ON);
  case REQUIRED_BY:
    return getPackageItem(parent).canFetchChildren(PackageItem::REQUIRED_BY);
  }
}

void PackageModel::fetchMore(const QModelIndex& parent)
{
  switch (m_displayMode) {
  case FLAT:
  default:
    return;
  case DEPENDS_ON:
    getPackageItem(parent).fetchDepencies(PackageItem::DEPENDS_ON);
    return;
  case REQUIRED_BY:
    getPackageItem(parent).fetchDepencies(PackageItem::REQUIRED_BY);
    return;
  }
}

void PackageModel::sort(int column, Qt::SortOrder order)
{
//  std::cout << "sort column " << column << " in order " << order << std::endl;

  if (column != m_sortColumn || order != m_sortOrder) {
    if (m_displayMode == FLAT)
      emit layoutAboutToBeChanged();
    else beginResetModel();
    m_sortColumn = column;
    m_sortOrder  = order;
    sort();
    if (m_displayMode == FLAT)
      emit layoutChanged();
    else {
      //TODO: reset here requires reset model -> maybe sort child-list instead
      m_rootItem.reset(new PackageItem(m_columnSortedlistOfPackages,
                                       m_displayMode == DEPENDS_ON ? PackageItem::DEPENDS_ON : PackageItem::REQUIRED_BY));
      endResetModel();
    }
  }
}

void PackageModel::beginResetRepository()
{
  beginResetModel();
  m_listOfPackages.clear();
  m_columnSortedlistOfPackages.clear();
}

void PackageModel::endResetRepository()
{
  const PackageRepository::TListOfPackages& data = m_packageRepo.getPackageList(m_filterPackagesNotInThisGroup);
  m_listOfPackages.reserve(data.size());
  for (PackageRepository::TListOfPackages::const_iterator it = data.begin(); it != data.end(); ++it) {
      if (m_filterPackagesNotInstalled == false || (*it)->installed()) {
        if (m_filterRegExp.isEmpty()) {
          m_listOfPackages.push_back(*it);
        }
        else {
          switch (m_filterColumn) {
            case ctn_PACKAGE_NAME_COLUMN:
              if (m_filterRegExp.indexIn((*it)->name) != -1) m_listOfPackages.push_back(*it);
              break;
            case ctn_PACKAGE_DESCRIPTION_FILTER_NO_COLUMN:
              if (m_filterRegExp.indexIn((*it)->description) != -1) m_listOfPackages.push_back(*it);
              break;
            default:
              m_listOfPackages.push_back(*it);
          }
        }
      }
  }
  m_columnSortedlistOfPackages.reserve(data.size());
  m_columnSortedlistOfPackages = m_listOfPackages;
  sort();
  if (m_displayMode == FLAT)
    m_rootItem.reset(createDummyRoot());
  else
    m_rootItem.reset(new PackageItem(m_columnSortedlistOfPackages,
                                     m_displayMode == DEPENDS_ON ? PackageItem::DEPENDS_ON : PackageItem::REQUIRED_BY));
  endResetModel();
}

int PackageModel::getPackageCount() const
{
  return m_listOfPackages.size();
}

const PackageRepository::PackageData* PackageModel::getData(const QModelIndex& index) const
{
  if (index.isValid() == false || index.internalPointer() == NULL)
    return NULL;

  switch (m_displayMode) {
  case FLAT: {
    return static_cast<const PackageRepository::PackageData*const>(index.internalPointer());
  }
  case DEPENDS_ON:
  case REQUIRED_BY: {
    const PackageItem*const data = static_cast<const PackageItem*const>(index.internalPointer());
    return &data->getPackage();
  }
  default:
    assert(false);
    return NULL;
  }
}

void PackageModel::switchDisplayMode(PackageModel::EDisplayMode newMode)
{
  beginResetRepository();
  m_displayMode = newMode;
  endResetRepository();
}

void PackageModel::applyFilter(bool packagesNotInstalled, const QString& group)
{
//  std::cout << "apply new group filter " << (packagesNotInstalled ? "true" : "false") << ", " << group.toStdString() << std::endl;

  beginResetRepository();
  m_filterPackagesNotInstalled   = packagesNotInstalled;
  m_filterPackagesNotInThisGroup = group;
  endResetRepository();
}

void PackageModel::applyFilter(const int filterColumn)
{
  applyFilter(filterColumn, m_filterRegExp.pattern());
}

void PackageModel::applyFilter(const QString& filterExp)
{
  applyFilter(m_filterColumn, filterExp);
}

void PackageModel::applyFilter(const int filterColumn, const QString& filterExp)
{
  assert(filterExp.isNull() == false);
//  std::cout << "apply new column filter " << filterColumn << ", " << filterExp.toStdString() << std::endl;

  beginResetRepository();
  m_filterColumn = filterColumn;
  m_filterRegExp.setPattern(filterExp);
  endResetRepository();
}

PackageItem& PackageModel::getPackageItem(const QModelIndex& index) const
{
  if (index.isValid()) {
    PackageItem*const branch = static_cast<PackageItem*const>(index.internalPointer());
    if (branch) return *branch;
  }
  return *m_rootItem;
}

const QIcon& PackageModel::getIconFor(const PackageRepository::PackageData& package) const
{
  switch (package.status)
  {
    case ectn_FOREIGN:
      return m_iconForeign;
    case ectn_FOREIGN_OUTDATED:
      return m_iconForeignOutdated;
    case ectn_OUTDATED:
      if (package.explicitlyInstalled) return m_iconOutdatedByUser;
      return m_iconOutdated;
    case ectn_NEWER:
      if (package.explicitlyInstalled) return m_iconNewerByUser;
      return m_iconNewer;
    case ectn_INSTALLED:
      // Does no other package depend on this package ? (unrequired package list)
      if (package.required)
      {
        if (package.explicitlyInstalled) return m_iconInstalledByUser;
        return m_iconInstalled;
      }
      else
      {
        if (package.explicitlyInstalled) return m_iconInstalledUnrequiredByUser;
        return m_iconInstalledUnrequired;
      }
      break;
    case ectn_NON_INSTALLED:
//      if (package.required == false) std::cout << "not installed not required" << std::endl; // doesn't happen with pacman
      return m_iconNotInstalled;
    default:
      assert(false);
  }
}


struct TSort0 {
  bool operator()(const PackageRepository::PackageData* a, const PackageRepository::PackageData* b) const {
    if (a->explicitlyInstalled > b->explicitlyInstalled) return true;
    if (a->explicitlyInstalled == b->explicitlyInstalled) {
      if (a->status < b->status) return true;
      if (a->status == b->status) {
        if (a->required < b->required) return true;
        if (a->required == b->required) {
          return a->name < b->name;
        }
      }
    }
    return false;
  }
};

struct TSort2 {
  bool operator()(const PackageRepository::PackageData* a, const PackageRepository::PackageData* b) const {
    const int cmp = Package::rpmvercmp(a->version.toLatin1().data(), b->version.toLatin1().data());
    if (cmp < 0) return true;
    if (cmp == 0) {
      return a->name < b->name;
    }
    return false;
  }
};

struct TSort3 {
  bool operator()(const PackageRepository::PackageData* a, const PackageRepository::PackageData* b) const {
    if (a->repository < b->repository) return true;
    if (a->repository == b->repository) {
      return a->name < b->name;
    }
    return false;
  }
};

struct TSort4 {
  bool operator()(const PackageRepository::PackageData* a, const PackageRepository::PackageData* b) const {
    if (a->popularity > b->popularity) return true;
    if (a->popularity == b->popularity) {
      return a->name < b->name;
    }
    return false;
  }
};

void PackageModel::sort()
{
  switch (m_sortColumn) {
  case ctn_PACKAGE_NAME_COLUMN:
    m_columnSortedlistOfPackages = m_listOfPackages;
    return;
  case ctn_PACKAGE_ICON_COLUMN:
    qSort(m_columnSortedlistOfPackages.begin(), m_columnSortedlistOfPackages.end(), TSort0());
    return;
  case ctn_PACKAGE_VERSION_COLUMN:
    qSort(m_columnSortedlistOfPackages.begin(), m_columnSortedlistOfPackages.end(), TSort2());
    return;
  case ctn_PACKAGE_REPOSITORY_COLUMN:
    qSort(m_columnSortedlistOfPackages.begin(), m_columnSortedlistOfPackages.end(), TSort3());
    return;
  case ctn_PACKAGE_POPULARITY_COLUMN:
    qSort(m_columnSortedlistOfPackages.begin(), m_columnSortedlistOfPackages.end(), TSort4());
    return;
  default:
    return;
  }
}

int PackageModel::transformRowIndex(int row, int rowCount) const
{
  switch (m_sortOrder) {
  case Qt::AscendingOrder:
    return row;
  case Qt::DescendingOrder:
    return rowCount - row - 1;
  }
  return -1;
}

PackageItem*PackageModel::createDummyRoot()
{
  return new PackageItem(PackageItem::TPkgVec(), PackageItem::DEPENDS_ON);
}
