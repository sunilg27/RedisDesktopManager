#include "KeyModel.h"

#include <QEventLoop>
#include <QTimer>
#include <QVariant>

#include "RedisKeyItem.h"
#include "Command.h"
#include "ConnectionBridge.h"

KeyModel::KeyModel(ConnectionBridge * db, const QString &keyName, int dbIndex)
	: db(db), keyName(keyName), dbIndex(dbIndex), keyType(Empty), keyTypeString("empty")
{	
}

KeyModel::~KeyModel(void)
{
	if (db != nullptr) {
		db->disconnect(this);
	}
}

QString KeyModel::getKeyName()
{
	return keyName;
}

void KeyModel::getValue()
{
	QString command;

	switch (keyType)
	{
	case KeyModel::String:
		command = QString("get %1").arg(keyName);
		break;

	case KeyModel::Hash:		
		command = QString("hgetall %1").arg(keyName);
		break;

	case KeyModel::List:
		command = QString("LRANGE %1 0 -1").arg(keyName);		
		break;

	case KeyModel::Set:
		command = QString("SMEMBERS %1").arg(keyName);				
		break;

	case KeyModel::ZSet:		
		command = QString("ZRANGE %1 0 -1 WITHSCORES").arg(keyName);
		break;
	}	

	if (command.isEmpty()) {
		emit valueLoaded(QVariant());
		return;
	} else {
		db->addCommand(Command(command, this, CALLMETHOD("loadedValue"), dbIndex));
	}
}

void KeyModel::loadedValue(const QVariant& value)
{
	emit valueLoaded(value);
}

void KeyModel::loadedType(const QVariant& result)
{
	QString t = result.toString();

	keyType = None;
	keyTypeString = t;

	if (t == "string")
		keyType = String;

	if (t == "hash") 
		keyType = Hash;

	if (t == "list")
		keyType = List;

	if (t == "set") 
		keyType = Set;

	if (t == "zset") 
		keyType = ZSet;

	emit keyTypeLoaded(keyType);

	return;
}

void KeyModel::renameKey(const QString& newKeyName)
{	
	QString renameCommand = QString("RENAME %1 %2")
								.arg(keyName)
								.arg(newKeyName);
	
	db->addCommand(Command(renameCommand, this, CALLMETHOD("loadedRenameStatus"), dbIndex));
}

void KeyModel::loadedRenameStatus(const QVariant& result)
{
	QString resultString = (result.isNull()) ? "" : result.toString();

	if (resultString.at(0) != 'O')
		emit keyRenameError(resultString);
	else 
		emit keyRenamed();	
}

QString KeyModel::getKeyTypeString()
{
	return keyTypeString;
}