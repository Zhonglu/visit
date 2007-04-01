#ifndef QUERYOVERTIMEATTRIBUTES_H
#define QUERYOVERTIMEATTRIBUTES_H
#include <state_exports.h>
#include <string>
#include <AttributeSubject.h>
#include <QueryAttributes.h>

// ****************************************************************************
// Class: QueryOverTimeAttributes
//
// Purpose:
//    Attributes for queries over time.
//
// Notes:      Autogenerated by xml2atts.
//
// Programmer: xml2atts
// Creation:   Wed May 26 10:23:00 PDT 2004
//
// Modifications:
//   
// ****************************************************************************

class STATE_API QueryOverTimeAttributes : public AttributeSubject
{
public:
    enum TimeType
    {
        Cycle,
        DTime,
        Timestep
    };

    QueryOverTimeAttributes();
    QueryOverTimeAttributes(const QueryOverTimeAttributes &obj);
    virtual ~QueryOverTimeAttributes();

    virtual void operator = (const QueryOverTimeAttributes &obj);
    virtual bool operator == (const QueryOverTimeAttributes &obj) const;
    virtual bool operator != (const QueryOverTimeAttributes &obj) const;

    virtual const std::string TypeName() const;
    virtual bool CopyAttributes(const AttributeGroup *);
    virtual AttributeSubject *CreateCompatible(const std::string &) const;
    virtual AttributeSubject *NewInstance(bool) const;

    // Property selection methods
    virtual void SelectAll();
    void SelectQueryAtts();

    // Property setting methods
    void SetTimeType(TimeType timeType_);
    void SetStartTimeFlag(bool startTimeFlag_);
    void SetStartTime(int startTime_);
    void SetEndTimeFlag(bool endTimeFlag_);
    void SetEndTime(int endTime_);
    void SetStride(int stride_);
    void SetCreateWindow(bool createWindow_);
    void SetWindowId(int windowId_);
    void SetQueryAtts(const QueryAttributes &queryAtts_);

    // Property getting methods
    TimeType              GetTimeType() const;
    bool                  GetStartTimeFlag() const;
    int                   GetStartTime() const;
    bool                  GetEndTimeFlag() const;
    int                   GetEndTime() const;
    int                   GetStride() const;
    bool                  GetCreateWindow() const;
    int                   GetWindowId() const;
    const QueryAttributes &GetQueryAtts() const;
          QueryAttributes &GetQueryAtts();

    // Persistence methods
    virtual bool CreateNode(DataNode *node, bool completeSave, bool forceAdd);
    virtual void SetFromNode(DataNode *node);

    // Enum conversion functions
    static std::string TimeType_ToString(TimeType);
    static bool TimeType_FromString(const std::string &, TimeType &);
protected:
    static std::string TimeType_ToString(int);
public:

    // Keyframing methods
    virtual std::string               GetFieldName(int index) const;
    virtual AttributeGroup::FieldType GetFieldType(int index) const;
    virtual std::string               GetFieldTypeName(int index) const;
    virtual bool                      FieldsEqual(int index, const AttributeGroup *rhs) const;

private:
    int             timeType;
    bool            startTimeFlag;
    int             startTime;
    bool            endTimeFlag;
    int             endTime;
    int             stride;
    bool            createWindow;
    int             windowId;
    QueryAttributes queryAtts;
};

#endif
