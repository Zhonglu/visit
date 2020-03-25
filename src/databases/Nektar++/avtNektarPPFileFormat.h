// Copyright (c) Lawrence Livermore National Security, LLC and other VisIt
// Project developers.  See the top-level LICENSE file for dates and other
// details.  No copyright assignment is required to contribute to VisIt.

// ************************************************************************* //
//                            avtNektarPPFileFormat.h                        //
// ************************************************************************* //

#ifndef AVT_NektarPP_FILE_FORMAT_H
#define AVT_NektarPP_FILE_FORMAT_H

#include <avtMTSDFileFormat.h>

#include <vtkUnstructuredGrid.h>

#include <LibUtilities/BasicUtils/SessionReader.h>
#include <MultiRegions/ExpList.h>
#include <SpatialDomains/Geometry.h>
#include <SpatialDomains/MeshGraph.h>

#include <vector>

class vtkDataSet;
class vtkDataArray;
class DBOptionsAttributes;

// ****************************************************************************
//  Class: avtNektarPPFileFormat
//
//  Purpose:
//      Reads in NektarPP files as a plugin to VisIt.
//
//  Programmer: allen -- generated by xml2avt
//  Creation:   Fri Nov 7 13:51:33 PST 2014
//
// ****************************************************************************

class avtNektarPPFileFormat : public avtMTSDFileFormat
{
  public:
                       avtNektarPPFileFormat(const char *, DBOptionsAttributes *);
    virtual           ~avtNektarPPFileFormat() {;};

    virtual void       Initialize();
    virtual void       ActivateTimestep(int ts);

    //
    // This is used to return unconvention data -- ranging from material
    // information to information about block connectivity.
    //
    // virtual void      *GetAuxiliaryData(const char *var, int timestep, 
    //                                     const char *type, void *args, 
    //                                     DestructorFunction &);
    //

    //
    // If you know the times and cycle numbers, overload this function.
    // Otherwise, VisIt will make up some reasonable ones for you.
    //
    virtual void        GetCycles(std::vector<int> &cycles);
    virtual void        GetTimes(std::vector<double> &times);
    //

    virtual int            GetNTimesteps(void);

    virtual const char    *GetType(void)   { return "NektarPP"; };
    virtual void           FreeUpResources(void); 

    virtual vtkDataSet    *GetMesh(int, const char *);
    virtual vtkDataArray  *GetVar(int, const char *);
    virtual vtkDataArray  *GetVectorVar(int, const char *);

  protected:
    Nektar::LibUtilities::SessionReaderSharedPtr m_vSession;

    // DATA MEMBERS
    std::vector<int>    m_cycles;
    std::vector<double> m_times;

    int m_refinement;
    bool m_ignoreCurvedElements;

    std::string m_meshFile;
    std::string m_fieldFile;

    std::string vectorVarComponents[3];

    int m_nElements;
    std::vector<std::string> m_scalarVarNames;
    std::vector<std::string> m_vectorVarCompNames;

    virtual void           PopulateDatabaseMetaData(avtDatabaseMetaData *, int);

    vtkDataSet *refinedDataSet;

    std::string GetNektarFileAsXMLString( std::string var );

    double GetNektarVar( const Nektar::Array<Nektar::OneD, Nektar::NekDouble> &coords,
                         const int nt_el,
                         const int index ) const;
  
    void GetNektarVectorVar( const Nektar::Array<Nektar::OneD, Nektar::NekDouble> &coords,
                             const int nt_el,
                             double *vec ) const;

    Nektar::MultiRegions::ExpListSharedPtr nektar_field[3];
};

#endif
