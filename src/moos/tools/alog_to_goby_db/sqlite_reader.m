function [out] = sqlite_reader(query, database)

dbid = mksqlite(0, 'open', database);
% array of struct
result = mksqlite(dbid, query);
names = fieldnames(result);
mksqlite(dbid, 'close');

% convert to struct of arrays, NOT array of structs
result_cell = struct2cell(result)';

for i = 1:size(names)
    is_repeated_number = false;
    % swap empty cells (NULL from SQL) for NaN
    for j = 1:length(result_cell(:,i))
        if(isempty(result_cell{j,i}))
            result_cell{j,i} = NaN;
        end
        % attempt to turn repeated number strings into numbers
        if isstr(result_cell{j,i})
            [num_vector result] = str2num(result_cell{j,i});
            if result
                result_cell{j,i} = num_vector;
                if(length(num_vector) > 1)
                    is_repeated_number = true;
                end
            end
        end
    end
    
    % try to convert to a matrix if
    % it's not a repeated number (array inside cell)
    if ~is_repeated_number
        try
            out.(names{i}) = cell2mat(result_cell(:,i));
            leave_as_cell = false;
        catch err
            leave_as_cell = true;
        end
    else
        leave_as_cell = true;
    end
    
    if leave_as_cell
        out.(names{i}) = result_cell(:,i);
    end
    
end

if size(names) == 1
   out = out.(names{1});
end

end