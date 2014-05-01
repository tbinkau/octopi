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

#ifndef OCTOPI_PACKAGEREPOSITORY_H
#define OCTOPI_PACKAGEREPOSITORY_H

#include <vector>
#include <memory>
#include <cassert>
#include <QList>

#include "package.h"


/**
 * @brief Central data storage for package data
 */
class PackageRepository
{
public:
  class PackageData;
  typedef QList<PackageData*> TListOfPackages;

public:
  ////////////////////////
  /**
   * @brief The IDependency class used for notification of dependent models
   */
  class IDependency {
  public:
    virtual void beginResetRepository() = 0;
    virtual void endResetRepository() = 0;
  };

  ////////////////////////
  class PackageGuard;

  ////////////////////////
  /**
   * @brief Holds data of one package + a few convenience functions
   */
  class PackageData {
  public:
    friend class PackageRepository::PackageGuard;
    typedef TListOfPackages TDependencyVec;

  public:
    /**
     * @brief PackageData constructor
     * @param package    = parsed data from pacman (e.g.)
     * @param isRequired = false if package is not required by other packages installed, or true otherwise
     */
    PackageData(const PackageListData& package, const bool isRequired, const bool isManagedByYaourt,
                const bool wasExplicitlyInstalled);

    inline bool installed() const {
      return status != ectn_NON_INSTALLED;
    }

    inline const TDependencyVec* getDependsOn() const {
      return dependsOn.get();
    }

    inline const TDependencyVec* getRequiredBy() const {
      return requiredBy.get();
    }

  private:
    inline void setDependsOn(const TDependencyVec* packages) {
//      std::cout << "set dependencies for " << name.toStdString() << " to " << dependencies << std::endl;
      this->dependsOn.reset(packages);
    }
    inline void resetRequiredBy() {
      this->requiredBy.reset(new TDependencyVec());
    }
    inline void addRequiredBy(PackageData& pkg) {
      assert(this->requiredBy.get() != NULL);
      this->requiredBy->push_back(&pkg);
    }

  public:
    const bool    required;
    const bool    managedByYaourt; // yaourt packages must not be in any group
    const bool    explicitlyInstalled;
    const QString name;
    const QString repository;
    const QString version;
    const QString description;
    const QString outdatedVersion;
    const double  downloadSize;
    const PackageStatus status;
    const int     popularity; // -1 for non AUR

  private:
    std::auto_ptr<const TDependencyVec> dependsOn;
    std::auto_ptr<TDependencyVec>       requiredBy;
  };

  ////////////////////////
  /**
   * @brief The PackageGuard class enables access to specific private functions
   */
  class PackageGuard {
    friend class PackageRepository;

    inline static void setDependencies(PackageData& pkg, const PackageData::TDependencyVec*const dependencies);
    inline static void resetRequirements(PackageData& pkg);
    inline static void addRequirement(PackageData& pkg, PackageData& dependsOnPkg);
  };

  ////////////////////////
  /**
   * @brief The Group class holds name and members of a package group
   */
  class Group {
  public:
    Group(const QString& name);

    const QString& getName();
    bool memberListEquals(const QStringList& packagelist);
    void addPackage(PackageData& package);
    void invalidateList();

    const TListOfPackages* getPackageList() const;

  private:
    QString name;
    TListOfPackages* m_listOfPackages; // WEAK ptr PackageData*
  };
  ////////////////////////

public:
  PackageRepository();

  void registerDependency(IDependency& depends);
  void setData(const QList<PackageListData>*const listOfPackages, const QSet<QString>& unrequiredPackages,
               const QSet<QString>& explicitlyInstalledPackages);
  void setAURData(const QList<PackageListData>*const listOfForeignPackages, const QSet<QString>& unrequiredPackages);
  void setPackageDependencies(const QList<std::pair<PackageData*, QStringList> >& dependencies);
  bool setPackageRequirements(bool forceSuccessful);
  void checkAndSetGroups(const QStringList& listOfGroups);
  void checkAndSetMembersOfGroup(const QString& group, const QStringList& members);

  const TListOfPackages& getPackageList() const;
  const TListOfPackages& getPackageList(const QString& group) const;
  PackageData*           getFirstPackageByName(const QString name) const;

private:
  std::vector<IDependency*> m_dependingModels;
  TListOfPackages           m_listOfPackages;       // sorted qlist of all packages
  TListOfPackages           m_listOfYaourtPackages; // sorted qlist of all yaourt packages
  QList<Group*>             m_listOfGroups;         // sorted list of all pacman package groups
  bool memberListOfGroupsEquals(const QStringList& listOfGroups);
};


void PackageRepository::PackageGuard::setDependencies(PackageData& pkg, const PackageData::TDependencyVec*const dependencies) {
  pkg.setDependsOn(dependencies);
}
void PackageRepository::PackageGuard::resetRequirements(PackageData& pkg) {
  pkg.resetRequiredBy();
}
void PackageRepository::PackageGuard::addRequirement(PackageData& pkg, PackageData& dependsOnPkg) {
  pkg.addRequiredBy(dependsOnPkg);
}

#endif // OCTOPI_PACKAGEREPOSITORY_H
