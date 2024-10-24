interface SortProps {
  selected: string;
  onArrowUp: (column_key: string) => void;
  onArrowDown: (column_key: string) => void;
  column_key: string;
}

/**
 * Sort component for rendering sort arrows on table columns
 * @component
 * @param {Object} props - Props for the Sort component
 * @param {string} props.selected - The selected column for sorting
 * @param {function} props.onArrowUp - Handler for arrow up click
 * @param {function} props.onArrowDown - Handler for arrow down click
 * @param {string} props.column_key - The column key for the Sort component
 */
const Sort = ({ selected, onArrowDown, onArrowUp, column_key }: SortProps) => {
  return (
    <div className="flex flex-col ml-0.5">
      <div
        onClick={() => onArrowUp(column_key)}
        className={
          (selected === column_key + "_up"
            ? "border-b-danube-400 border-t-danube-200 hover:border-b-danube-400 hover:border-t-danube-200 "
            : "border-t-danube-200 border-b-danube-200 hover:border-t-danube-300 hover:border-b-danube-300 ") +
          "cursor-pointer w-0 h-0 border-l-[7px] border-l-transparent border-b-[8px] border-r-[7px] border-r-transparent right-0 bottom-[13px] mb-0.5 hover:border-t-danube-300 hover:border-b-danube-300"
        }
      ></div>
      <div
        onClick={() => onArrowDown(column_key)}
        className={
          (selected === column_key + "_down"
            ? "border-t-danube-400 border-b-danube-200 hover:border-t-danube-400 hover:border-b-danube-200 "
            : "border-t-danube-200 border-b-danube-200 hover:border-t-danube-300 hover:border-b-danube-300 ") +
          "cursor-pointer w-0 h-0 border-l-[7px] border-l-transparent border-t-[8px] border-r-[7px] border-r-transparent right-0 bottom-[2px] mt-0.5"
        }
      ></div>
    </div>
  );
};

export default Sort;
