import csv

class DBUsers:
    """
    Class to interface with the database containing users.
    Current implementation is a CSV file.
    """
    def __init__(self, path='data/users.csv'):
        """
        Database config
        """
        self.path = path

    def get_records(self):
        """
        Get a fresh list of records from the database
        """
        with open(self.path, 'r') as db_file:
            headers = []
            records = []
            for row_index, values in enumerate(csv.reader(db_file)):
                if row_index == 0:
                    # Retrieve headers
                    headers = values
                    continue

                record = {}
                for header_index in range(len(headers)):
                    header = headers[header_index]
                    record[header] = values[header_index]
                
                records.append(record)
        
        return records

    def get_by_username(self, username):
        """
        Get record by username
        """
        records = self.get_records()
        if len(records) == 0:
            return None

        for record in records:
            if record.get('username') == username:
                return record

        return None

    def get_by_key(self, key):
        """
        Get record by API key
        """
        records = self.get_records()
        if len(records) == 0:
            return None

        for record in records:
            if record.get('key') == key:
                return record

        return None

    def get_by_id(self, id):
        """
        Get record by telegram user ID
        """
        records = self.get_records()
        if len(records) == 0:
            return None

        for record in records:
            if record.get('user_id') == str(id):
                return record
        
        return None

    def edit_record(self, selector_field_name, selector_field_value, edit_field_name, edit_field_new_value) -> bool:
        """
        Edit a record. Returns a boolean value indicating the function's success
        """
        records = self.get_records()
        if len(records) == 0:
            return False

        for record in records:
            if record.get(selector_field_name) == str(selector_field_value):
                record[edit_field_name] = edit_field_new_value
                success = self.write_records(records) 
                return success

        return False

    def write_records(self, records: list) -> bool:
        """
        Writes a list of records into the db file.
        Headers should not be part of the list.
        Returns a boolean value indicating the function's success
        """
        # Take first item as reference for the headers
        headers = list(records[0].keys())
        
        with open(self.path, 'w', newline='') as db_file:
            writer = csv.writer(db_file)
            writer.writerow(headers)

            for record in records:
                row = []

                for header in headers:
                    row.append(record[header])

                writer.writerow(row)

        return True
